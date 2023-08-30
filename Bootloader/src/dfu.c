#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"
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

/**@brief Function that sets the stack pointer and starts executing a particular address.
 *
 * @param[in]  new_msp  The new value to set in the main stack pointer.
 * @param[in]  addr     The address to execute.
 */
static void jump_to_addr(uint32_t new_msp, uint32_t addr)
{
    __set_MSP(new_msp);
    ((void (*)(void))addr)();
}


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
    const uint32_t vector_table_addr = FLASH_APP_START_ADDR;

    const uint32_t current_isr_num = (__get_IPSR() & IPSR_ISR_Msk);
    const uint32_t new_msp         = *((uint32_t *)(vector_table_addr));                    // The app's Stack Pointer is found as the first word of the vector table.
    const uint32_t reset_handler   = *((uint32_t *)(vector_table_addr + sizeof(uint32_t))); // The app's Reset Handler is found as the second word of the vector table.

    __set_CONTROL(0x00000000);   // Set CONTROL to its reset value 0.
    __set_PRIMASK(0x00000000);   // Set PRIMASK to its reset value 0.
    __set_BASEPRI(0x00000000);   // Set BASEPRI to its reset value 0.
    __set_FAULTMASK(0x00000000); // Set FAULTMASK to its reset value 0.

    ASSERT(current_isr_num == 0); // If this is triggered, the CPU is currently in an interrupt.

    // Relocate the vector table
    // SCB->VTOR = vector_table_addr;

    // The CPU is in Thread mode (main context).
    jump_to_addr(new_msp, reset_handler); // Jump directly to the App's Reset Handler.
}
