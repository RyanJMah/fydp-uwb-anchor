#include "nrf_fstorage.h"
#include "nrf_log_ctrl.h"
#include "crc32.h"

#ifdef GL_BOOTLOADER
    #include "nrf_fstorage_nvmc.h"
#else
    #include "nrf_fstorage_sd.h"
#endif

#include "gl_log.h"
#include "flash_memory_map.h"
#include "flash_config_data.h"

/*************************************************************
 * MACROS
 ************************************************************/

// Number of bytes to pad the config data with so that it is a multiple of 4 bytes
#define PADDING_BYTES           ( sizeof(uint32_t) - (sizeof(FlashConfigData_t) % 4) )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/

/*
 * This is bad, but python ctypes can't align to 32 bits on
 * a 64 bit machine, so we do it like this I guess...
 */
typedef struct
{
    FlashConfigData_t data;
    uint8_t           padding[PADDING_BYTES];
} ConfData_WithPadding_t;

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NRF_FSTORAGE_DEF(nrf_fstorage_t g_app_fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_CONFIG_DATA_START_ADDR,
    .end_addr    = FLASH_CONFIG_DATA_END_ADDR + 1
};

static uint8_t g_is_initialized = 0;

static ConfData_WithPadding_t g_conf_with_padding;

FlashConfigData_t* gp_persistent_conf = &g_conf_with_padding.data;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static uint32_t _compute_crc(FlashConfigData_t *p_data)
{
    // Compute the crc32 of the config data, excluding the crc32 field
    return crc32_compute( (uint8_t*)p_data,
                           sizeof(FlashConfigData_t) - sizeof(uint32_t),
                           NULL );
}

static uint8_t _verify_crc(FlashConfigData_t *p_data)
{
    return p_data->crc32 == _compute_crc(p_data);
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t FlashConfigData_Init(void)
{
    if ( g_is_initialized )
    {
        return NRF_SUCCESS;
    }

    ret_code_t err_code;

    // Initialize the flash storage module

// Use SD for app, NVMC for bootloader
#ifdef GL_BOOTLOADER
    err_code = nrf_fstorage_init(&g_app_fstorage, &nrf_fstorage_nvmc, NULL);
#else
    err_code = nrf_fstorage_init(&g_app_fstorage, &nrf_fstorage_sd, NULL);
#endif

    require_noerr(err_code, exit);

    // Ready config data from flash
    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  FLASH_CONFIG_DATA_START_ADDR,
                                  &g_conf_with_padding,
                                  sizeof(FlashConfigData_t) );
    require_noerr(err_code, exit);

    // Verify the CRC of the config data
    if ( !_verify_crc(gp_persistent_conf) )
    {
        GL_LOG("REALLY BAD ERROR: corrupt config data, please re-flash device...\n");

        while (1)
        {
            // Do nothing...
        }
    }

    // Mark as initialized
    g_is_initialized = 1;

    FlashConfigData_Print();
    GL_LOG("\n");

exit:
    return err_code;
}

uint8_t FlashConfigData_IsInitalized(void)
{
    return g_is_initialized;
}

uint8_t FlashConfigData_Validate(void)
{
    if ( !g_is_initialized )
    {
        return 0;
    }

    return _verify_crc(gp_persistent_conf);
}

void FlashConfigData_Print(void)
{
    GL_LOG("FlashConfigData: \n");
    GL_LOG("  - fw_update_pending:      %d\n", gp_persistent_conf->fw_update_pending);
    GL_LOG("  - anchor_id:              %d\n", gp_persistent_conf->anchor_id);
    GL_LOG("  - socket_recv_timeout_ms: %d\n", gp_persistent_conf->socket_recv_timeout_ms);

    // Pointer to array containing the mac address
    uint8_t *p_mac_addr = gp_persistent_conf->mac_addr;

    GL_LOG("  - mac_addr:               %02X:%02X:%02X:%02X:%02X:%02X\n",
                p_mac_addr[0], p_mac_addr[1], p_mac_addr[2],
                p_mac_addr[3], p_mac_addr[4], p_mac_addr[5]  );

    GL_LOG("  - using_dhcp:             %d\n", gp_persistent_conf->using_dhcp);
    GL_LOG("  - crc32:                  %08X\n", gp_persistent_conf->crc32);
}

ret_code_t FlashConfigData_WriteBack(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    // Re-compute the CRC
    gp_persistent_conf->crc32 = _compute_crc(gp_persistent_conf);

    // Erase the page before writing to it
    err_code = nrf_fstorage_erase( &g_app_fstorage,
                                   FLASH_CONFIG_DATA_START_ADDR,
                                   1,
                                   NULL );
    require_noerr(err_code, exit);

    // Write the config data to the swap page
    err_code = nrf_fstorage_write( &g_app_fstorage,
                                   FLASH_CONFIG_DATA_START_ADDR,
                                   &g_conf_with_padding,
                                   sizeof(g_conf_with_padding),
                                   NULL );
    require_noerr(err_code, exit);

    GL_LOG("Successfully wrote config data to flash!\n");

exit:
    return err_code;
}

ret_code_t FlashConfigData_Deinit(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    // De-initialize the flash storage module
    err_code = nrf_fstorage_uninit(&g_app_fstorage, NULL);
    require_noerr(err_code, exit);

exit:
    return err_code;
}
