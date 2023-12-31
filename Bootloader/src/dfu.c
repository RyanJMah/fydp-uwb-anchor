#include "flash_config_data.h"
#include "gl_error.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_sdm.h"
#include "nrf_log_ctrl.h"
#include "crc32.h"
#include "nrf.h"

#include "flash_memory_map.h"
#include "dfu_messages.h"
#include "dfu.h"

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NRF_FSTORAGE_DEF(nrf_fstorage_t g_app_code_fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_APP_START_ADDR,
    .end_addr    = FLASH_APP_END_ADDR + 1
};

static nrf_fstorage_t* g_fstorage = NULL;

static uint8_t g_is_initialized = 0;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t DFU_Init(void)
{
    if ( g_is_initialized )
    {
        return NRF_SUCCESS;
    }

    ret_code_t err_code;

    // Initialize the flash storage module
    err_code = nrf_fstorage_init(&g_app_code_fstorage, &nrf_fstorage_nvmc, NULL);
    require_noerr(err_code, exit);

    // Mark as initialized
    g_is_initialized = 1;

exit:
    return err_code;
}

ret_code_t DFU_Deinit(void)
{
    ret_code_t err_code;

    // Uninitialize the flash storage module
    err_code = nrf_fstorage_uninit(&g_app_code_fstorage, NULL);
    require_noerr(err_code, exit);

    // Mark as uninitialized
    g_is_initialized = 0;

exit:
    return err_code;
}

void DFU_SetUpdateType(DFU_UpdateType_t update_type)
{
    switch (update_type)
    {
        case DFU_UPDATE_TYPE_APP_CODE:
        {
            g_fstorage = &g_app_code_fstorage;
            break;
        }

        case DFU_UPDATE_TYPE_CONFIG_DATA:
        {
            g_fstorage = FlashConfigData_GetFStorage();
            break;
        }

        default:
        {
            GL_FATAL_ERROR();
        }
    }
}

ret_code_t DFU_EraseCurrentImg(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    // Erase all the pages

    uint32_t num_pages = (g_fstorage->end_addr - g_fstorage->start_addr) / FLASH_PAGE_SIZE;

    err_code = nrf_fstorage_erase(g_fstorage, g_fstorage->start_addr, num_pages, NULL);
    require_noerr(err_code, exit);

exit:
    return err_code;
}

bool DFU_ValidateChunk(DFU_ChunkMsg_t *chunk)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    // Calculate the CRC32 of the chunk
    uint32_t calculated_crc32 = crc32_compute( chunk->chunk_data,
                                               chunk->chunk_num_bytes,
                                               NULL );

    GL_LOG("Calculated CRC32: 0x%08X\n", calculated_crc32);
    GL_LOG("Received CRC32:   0x%08X\n", chunk->chunk_crc32);

    // Compare the CRC32s
    if ( calculated_crc32 != chunk->chunk_crc32 )
    {
        return false;
    }
    else
    {
        return true;
    }
}

ret_code_t DFU_ValidateImage(DFU_MetadataMsg_t* p_metadata, bool* out_ok)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;
    bool ok = false;

    static uint8_t buf[DFU_CHUNK_SIZE];

    // Calculate the CRC32 of the image
    uint32_t crc32 = 0;

    for (uint32_t i = 0; i < p_metadata->img_num_chunks; i++)
    {
        // Read the chunk from flash
        err_code = nrf_fstorage_read( g_fstorage,
                                      g_fstorage->start_addr + i*DFU_CHUNK_SIZE,
                                      buf,
                                      sizeof(buf) );
        require_noerr(err_code, exit);

        // Calculate the CRC32 of the chunk
        crc32 = crc32_compute( buf,
                               sizeof(buf),
                               &crc32 );
    }

    ok = (crc32 == p_metadata->img_crc);

    GL_LOG("Calculated CRC32: 0x%08X\n", crc32);
    GL_LOG("Received CRC32:   0x%08X\n", p_metadata->img_crc);

exit:
    *out_ok = ok;
    return err_code;
}

ret_code_t DFU_WriteChunk(DFU_ChunkMsg_t *chunk)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    static uint8_t write_buff[DFU_CHUNK_SIZE];

    memcpy( write_buff, chunk->chunk_data, MAX(chunk->chunk_num_bytes, DFU_CHUNK_SIZE) );

    ret_code_t err_code;

    // Calculate the address of the chunk
    uint32_t chunk_addr = g_fstorage->start_addr + (chunk->chunk_num * DFU_CHUNK_SIZE);

    // Needs to be aligned to 4 bytes
    uint32_t num_bytes_to_write = chunk->chunk_num_bytes;
    if ( (num_bytes_to_write % 4) != 0 )
    {
        num_bytes_to_write += 4 - (chunk->chunk_num_bytes % 4);
    }

    // Write the chunk to flash
    err_code = nrf_fstorage_write( g_fstorage,
                                   chunk_addr,
                                   write_buff,
                                   num_bytes_to_write,
                                   NULL);
    require_noerr(err_code, exit);

exit:
    return err_code;
}


void DFU_JumpToApp(void)
{
    nrf_bootloader_app_start();
}
