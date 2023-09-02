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
 * MACROS
 ************************************************************/
#define APP_NUM_PAGES       ( FLASH_APP_SIZE / FLASH_PAGE_SIZE )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NRF_FSTORAGE_DEF(nrf_fstorage_t g_app_code_fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_APP_START_ADDR,
    .end_addr    = FLASH_APP_END_ADDR
};

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

ret_code_t DFU_EraseAppCode(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    // // Erase all the app code pages
    err_code = nrf_fstorage_erase(&g_app_code_fstorage, FLASH_APP_START_ADDR, APP_NUM_PAGES, NULL);
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

    // Needs to be enough stack for this buffer...
    uint8_t buf[1024];

    uint32_t crc32 = 0xFFFFFFFF;
    int32_t num_bytes_left = p_metadata->img_num_bytes;

    // Calculate the CRC32 of the image
    while ( num_bytes_left )
    {
        int32_t offset = p_metadata->img_num_bytes - num_bytes_left;
        require(offset >= 0, exit);

        uint32_t bytes_to_read = MIN(sizeof(buf), num_bytes_left);

        // Read a chunk of the image
        err_code = nrf_fstorage_read( &g_app_code_fstorage,
                                      FLASH_APP_START_ADDR + offset,
                                      buf,
                                      bytes_to_read );
        require_noerr(err_code, exit);

        // Calculate the CRC32 of the chunk
        crc32 = crc32_compute( buf,
                               bytes_to_read,
                               &crc32 );

        num_bytes_left -= bytes_to_read;
    }

    ok = (crc32 == p_metadata->img_crc);

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

    ret_code_t err_code;

    // Calculate the address of the chunk
    uint32_t chunk_addr = FLASH_APP_START_ADDR + (chunk->chunk_num * DFU_CHUNK_SIZE);

    // Needs to be aligned to 4 bytes
    uint32_t num_bytes_to_write = chunk->chunk_num_bytes + (4 - (chunk->chunk_num_bytes % 4));

    // Write the chunk to flash
    err_code = nrf_fstorage_write( &g_app_code_fstorage,
                                   chunk_addr,
                                   chunk->chunk_data,
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
