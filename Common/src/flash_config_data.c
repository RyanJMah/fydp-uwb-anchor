#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"

#include "gl_log.h"
#include "flash_memory_map.h"
#include "flash_config_data.h"

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NRF_FSTORAGE_DEF(nrf_fstorage_t app_fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_CONFIG_DATA_START_ADDR,
    .end_addr    = FLASH_CONFIG_DATA_END_ADDR
};

static uint8_t g_is_initialized = 0;

FlashConfigData_t g_config;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t FlashConfigData_Init(void)
{
    ret_code_t err_code;

    err_code = nrf_fstorage_init(&app_fstorage, &nrf_fstorage_nvmc, NULL);
    require_noerr(err_code, exit);

    err_code = nrf_fstorage_read(&app_fstorage, FLASH_CONFIG_DATA_START_ADDR, &g_config, sizeof(g_config));
    require_noerr(err_code, exit);

    g_is_initialized = 1;

    GL_LOG("Successfully read config data from flash!\n");
    FlashConfigData_Print();
    GL_LOG("\n");

exit:
    return err_code;
}

uint8_t FlashConfigData_IsInitalized(void)
{
    return g_is_initialized;
}

void FlashConfigData_Print(void)
{
    GL_LOG("FlashConfigData: \n");
    GL_LOG("  - swap_count:             %d\n", g_config.swap_count);
    GL_LOG("  - fw_update_pending:      %d\n", g_config.fw_update_pending);
    GL_LOG("  - anchor_id:              %d\n", g_config.anchor_id);
    GL_LOG("  - socket_recv_timeout_ms: %d\n", g_config.swap_count);

    GL_LOG("  - mac_addr:               %02X:%02X:%02X:%02X:%02X:%02X\n",
                g_config.mac_addr[0], g_config.mac_addr[1], g_config.mac_addr[2],
                g_config.mac_addr[3], g_config.mac_addr[4], g_config.mac_addr[5]  );

    GL_LOG("  - using_dhcp:             %d\n", g_config.using_dhcp);
}

