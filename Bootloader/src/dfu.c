#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"
#include "crc32.h"

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
