#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_sdm.h"
#include "nrf_log_ctrl.h"
#include "crc32.h"
#include "nrf.h"

#include "flash_memory_map.h"
#include "dfu.h"

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
    // if ( !g_is_initialized )
    // {
    //     return NRF_ERROR_INVALID_STATE;
    // }

    // ret_code_t err_code;

    // // Erase all the app code pages
    // err_code = nrf_fstorage_erase(&g_app_code_fstorage, FLASH_APP_START_ADDR, FLASH_APP_NUM_PAGES, NULL);
    return NRF_SUCCESS;
}


void DFU_JumpToApp(void)
{
    // if ( sd_softdevice_vector_table_base_set(FLASH_APP_START_ADDR) != NRF_SUCCESS )
    // {
    //     GL_LOG("Failed to set vector table base address.\r\n");

    //     NRF_LOG_FINAL_FLUSH();

    //     while (1)
    //     {

    //     }
    // }

    nrf_bootloader_app_start();
}
