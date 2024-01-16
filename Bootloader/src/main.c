#include "gl_error.h"
#include "macros.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "nrf_sdm.h"
#include "nrf_bootloader_info.h"

#include "gl_log.h"
#include "lan.h"
#include "dfu.h"
#include "dfu_messages.h"
#include "flash_config_data.h"
#include "sdk_errors.h"
#include <stdint.h>

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
// The chunk message is the largest message in the protocol
static uint8_t g_rx_buf[ sizeof(DFU_ChunkMsg_t) ] __attribute__((aligned(4)));

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static int32_t _nack_chunk(void)
{
    DFU_OkMsg_t nack =
    {
        .msg_type = DFU_MSG_TYPE_OK,
        .ok       = 0
    };

    int sock_err_code = LAN_Send(DFU_SOCK_NUM, (uint8_t*)&nack, sizeof(nack));
    require(sock_err_code > 0, exit);

exit:
    return sock_err_code;
}

static int32_t _ack_chunk(void)
{
    DFU_OkMsg_t ack =
    {
        .msg_type = DFU_MSG_TYPE_OK,
        .ok       = 1
    };

    int sock_err_code = LAN_Send(DFU_SOCK_NUM, (uint8_t*)&ack, sizeof(ack));
    require(sock_err_code > 0, exit);

exit:
    return sock_err_code;
}

static int32_t _recieve_chunk(void)
{
    int32_t sock_err_code = 0;
    int32_t bytes_received = 0;

    while ( bytes_received < sizeof(DFU_ChunkMsg_t) )
    {
        sock_err_code = LAN_Recv( DFU_SOCK_NUM,
                                  g_rx_buf + bytes_received,
                                  sizeof(g_rx_buf) - bytes_received );
        require(sock_err_code > 0, exit);

        bytes_received += sock_err_code;

        GL_LOG("Received %d bytes, %d remaining...\n", sock_err_code, sizeof(DFU_ChunkMsg_t) - bytes_received);
    }

exit:
    return sock_err_code;
}

static ret_code_t _handle_chunk(void)
{
    int32_t    sock_err_code = 0;
    ret_code_t err_code      = NRF_SUCCESS;

    uint8_t num_retries = 0;

    while ( num_retries++ < DFU_MAX_NUM_RETRIES )
    {
        sock_err_code = _recieve_chunk();
        require_action(sock_err_code > 0, exit, err_code = NRF_ERROR_INTERNAL);

        DFU_ChunkMsg_t* p_chunk = (DFU_ChunkMsg_t*)g_rx_buf;

        if ( p_chunk->msg_type != DFU_MSG_TYPE_CHUNK )
        {
            GL_LOG("Received invalid message type: 0x%02X, retrying\n", p_chunk->msg_type);

            _nack_chunk();
            require_action(sock_err_code > 0, exit, err_code = NRF_ERROR_INTERNAL);

            continue;
        }
        else if ( !DFU_ValidateChunk(p_chunk) )
        {
            GL_LOG("CRC mismatch, retrying...\n");

            _nack_chunk();
            require_action(sock_err_code > 0, exit, err_code = NRF_ERROR_INTERNAL);

            continue;
        }
        else // Chunk is good
        {
            _ack_chunk();
            require_action(sock_err_code > 0, exit, err_code = NRF_ERROR_INTERNAL);

            // Write the chunk to flash
            err_code = DFU_WriteChunk(p_chunk);
            require_noerr(err_code, exit);

            // Success, move on to the next chunk
            break;
        }
    }

exit:
    return err_code;
}

/*************************************************************
 * MAIN
 ************************************************************/
int main(void)
{
    GL_LOG_INIT();

    GL_LOG("ENTERING BOOTLOADER!\n");

    // This needs to be here, idk exactly what it does, but it does some weird stuff that is necessary (apparently)
    nrf_bootloader_mbr_addrs_populate();

    GL_LOG("Reading config data from flash...\n");
    FlashConfigData_Init();

    if ( !gp_persistent_conf->fw_update_pending )
    {
        FlashConfigData_Deinit();

        GL_LOG("Jumping to app!\n");
        DFU_JumpToApp();
    }

    // If we're here, the fw_update_pending flag is set, start DFU

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Connect to DFU server
    LAN_Init( NULL );

    GL_LOG("Connecting to dfu server...\n");

    ret_code_t err_code      = NRF_SUCCESS;
    int        sock_err_code = 0;

    // Find a server to connect to

    // Try mDNS first
    for (uint8_t i = 0; i < NUM_FALLBACK_SERVERS; i++)
    {
        if ( Port_IsInvalid(gp_persistent_conf->server_port[i]) ||
             Hostname_IsInvalid(gp_persistent_conf->server_hostname[i]) )
        {
            continue;
        }

        ipv4_addr_t mdns_addr;
        sock_err_code = LAN_GetServerIPViaMDNS(gp_persistent_conf->server_hostname[i], &mdns_addr);

        if ( sock_err_code <= 0 )
        {
            GL_LOG("Failed to resolve hostname %d, trying next...\n", i);
            continue;
        }

        sock_err_code = LAN_Connect(DFU_SOCK_NUM, mdns_addr, DFU_SERVER_PORT);

        if ( sock_err_code > 0 )
        {
            // Success
            goto server_connected;
        }

        GL_LOG("Failed to connect to server %d, trying next...\n", i);
    }

    // Try raw IP addresses if mDNS fails
    for (uint8_t i = 0; i < NUM_FALLBACK_SERVERS; i++)
    {
        if ( Port_IsInvalid(gp_persistent_conf->server_port[i]) ||
             IPAddr_IsInvalid(gp_persistent_conf->server_ip_addr[i]) )
        {
            continue;
        }

        sock_err_code = LAN_Connect(DFU_SOCK_NUM, gp_persistent_conf->server_ip_addr[i], DFU_SERVER_PORT);

        if ( sock_err_code > 0 )
        {
            // Success
            goto server_connected;
        }

        GL_LOG("Failed to connect to server %d, trying next...\n", i);
    }

    if ( sock_err_code <= 0 )
    {
        GL_LOG("Failed to connect to any server, aborting...\n");
        goto err_handler;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Init DFU
server_connected:
    err_code = DFU_Init();
    require_noerr(err_code, err_handler);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Send READY message
    {
        GL_LOG("Sending READY...\n");

        DFU_ReadyMsg_t msg =
        {
            .msg_type = DFU_MSG_TYPE_READY
        };

        sock_err_code = LAN_Send(DFU_SOCK_NUM, (uint8_t*)&msg, sizeof(msg));
        require(sock_err_code > 0, err_handler);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Wait for METADATA message
    DFU_MetadataMsg_t metadata;

    GL_LOG("Waiting for METADATA...\n");

    sock_err_code = LAN_Recv(DFU_SOCK_NUM, g_rx_buf, sizeof(g_rx_buf));
    require(sock_err_code > 0, err_handler);

    metadata = *(DFU_MetadataMsg_t*)g_rx_buf;
    require(metadata.msg_type == DFU_MSG_TYPE_METADATA, err_handler);

    GL_LOG("UPDATE TYPE: %d\n", metadata.update_type);

    switch ( metadata.update_type )
    {
        case DFU_UPDATE_TYPE_APP_CODE:
        {
            GL_LOG("Received METADATA: APP CODE UPDATE: %d chunks to download...\n", metadata.img_num_chunks);
            break;
        }
        
        case DFU_UPDATE_TYPE_CONFIG_DATA:
        {
            GL_LOG("Received METADATA: CONFIG UPDATE: %d chunks to download...\n", metadata.img_num_chunks);
            break;
        }
    }

    DFU_SetUpdateType( metadata.update_type );    

    err_code = DFU_EraseCurrentImg();
    require_noerr(err_code, err_handler);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Send BEGIN message
    {
        GL_LOG("Sending BEGIN...\n");

        DFU_BeginMsg_t msg =
        {
            .msg_type = DFU_MSG_TYPE_BEGIN
        };

        sock_err_code = LAN_Send(DFU_SOCK_NUM, (uint8_t*)&msg, sizeof(msg));
        require(sock_err_code > 0, err_handler);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Receive and handle CHUNK messages
    for (uint32_t i = 0; i < metadata.img_num_chunks; i++)
    {
        GL_LOG("\nWaiting for CHUNK %d / %d...\n", i, metadata.img_num_chunks - 1);

        err_code = _handle_chunk();
        require_noerr(err_code, err_handler);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Validate the whole image
    GL_LOG("Validating image...\n");

    bool img_is_good;

    err_code = DFU_ValidateImage(&metadata, &img_is_good);
    require_noerr(err_code, err_handler);

    if ( !img_is_good )
    {
        GL_LOG("Image is bad!!!!\n");
        goto err_handler;
    }

    GL_LOG("Image is good!\n");
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Send CONFIRM message
    {
        GL_LOG("Sending CONFIRM...\n");

        DFU_ConfirmMsg_t msg =
        {
            .msg_type = DFU_MSG_TYPE_CONFIRM,
            .ok       = 1
        };

        sock_err_code = LAN_Send(DFU_SOCK_NUM, (uint8_t*)&msg, sizeof(msg));
        require(sock_err_code > 0, err_handler);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // De-init everything then jump to application code
    GL_LOG("Uninitializing...\n"); 

    // LAN_Init() resets the ethernet chip via the reset pin, so no need to deinit here

    err_code = DFU_Deinit();
    require_noerr(err_code, err_handler);

    if ( metadata.update_type == DFU_UPDATE_TYPE_CONFIG_DATA )
    {
        err_code = FlashConfigData_ReadBack();
        require_noerr(err_code, err_handler);
    }

    // Clear the fw_update_pending flag
    gp_persistent_conf->fw_update_pending = 0;

    err_code = FlashConfigData_WriteBack();
    require_noerr(err_code, err_handler);

    err_code = FlashConfigData_Deinit();
    require_noerr(err_code, err_handler);

    // Jump to application code
    GL_LOG("Rebooting...\n");
    NVIC_SystemReset();
    //////////////////////////////////////////////////////////////////////////////////////////////////////

err_handler:
    GL_LOG("DFU FATAL ERROR: err_code=%d, sock_err_code=%d\n", err_code, sock_err_code);
    GL_FATAL_ERROR();

    return 0;
}

