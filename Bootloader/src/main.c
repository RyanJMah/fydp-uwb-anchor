#include "gl_error.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"
#include "nrf_bootloader_info.h"

#include "gl_log.h"
#include "lan.h"
#include "dfu.h"
#include "dfu_messages.h"
#include "flash_config_data.h"

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
// The chunk message is the largest message in the protocol
static uint8_t g_rx_buf[ sizeof(DFU_ChunkMsg_t) ];

static uint32_t g_fw_up_retries = 0;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static int32_t _nack_chunk(DFU_ChunkMsg_t* chunk)
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

retry_fw_update:
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Connect to DFU server
    LAN_Init( NULL );

    GL_LOG("Connecting to dfu server...\n");

    ret_code_t err_code;
    int        sock_err_code;

    // Find a server to connect to
    for (uint8_t i = 0; i < NUM_FALLBACK_SERVERS; i++)
    {
        if ( Port_IsInvalid(gp_persistent_conf->server_port[i]) ||
             IPAddr_IsInvalid(gp_persistent_conf->server_ip_addr[i]) )
        {
            continue;
        }

        sock_err_code = LAN_Connect(DFU_SOCK_NUM, gp_persistent_conf->server_ip_addr[i], 6900);

        if ( sock_err_code > 0 )
        {
            // Success
            break;
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
    // Prepare for DFU
    err_code = DFU_Init();
    require_noerr(err_code, err_handler);

    err_code = DFU_EraseAppCode();
    require_noerr(err_code, err_handler);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // Send READY message
    {
        GL_LOG("Sending REQ...\n");

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
    uint8_t num_retries = 0;

    for (uint32_t i = 0; i < metadata.img_num_chunks; i++)
    {
    start_chunk:
        GL_LOG("Waiting for CHUNK %d...\n", i);

        sock_err_code = LAN_Recv(DFU_SOCK_NUM, g_rx_buf, sizeof(g_rx_buf));
        require(sock_err_code > 0, err_handler);

        DFU_ChunkMsg_t* p_chunk = (DFU_ChunkMsg_t*)g_rx_buf;

        if ( (p_chunk->msg_type != DFU_MSG_TYPE_CHUNK) ||
             !DFU_ValidateChunk(p_chunk) )
        {
            // NACK the chunk, server will retry
            _nack_chunk(p_chunk);

            if ( num_retries++ > DFU_MAX_NUM_RETRIES )
            {
                goto start_chunk;
            }
        }
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
        GL_LOG("Image is bad, retrying...\n");

        if ( g_fw_up_retries++ > DFU_MAX_NUM_RETRIES )
        {
            GL_LOG("Too many retries, aborting...\n");
            goto err_handler;
        }

        goto retry_fw_update;
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

    err_code = FlashConfigData_Deinit();
    require_noerr(err_code, err_handler);

    GL_LOG("Jumping to app...\n");
    DFU_JumpToApp();
    //////////////////////////////////////////////////////////////////////////////////////////////////////

err_handler:
    GL_LOG("DFU FATAL ERROR: err_code=%d, sock_err_code=%d\n", err_code, sock_err_code);
    GL_FATAL_ERROR();

    return 0;
}

