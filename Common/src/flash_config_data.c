#include "nrf_fstorage.h"
#include "nrf_fstorage_nvmc.h"

#include "gl_log.h"
#include "flash_memory_map.h"
#include "flash_config_data.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define FLASH_CONFIG_DATA_PAGE1_ADDR    ( FLASH_CONFIG_DATA_START_ADDR )
#define FLASH_CONFIG_DATA_PAGE2_ADDR    ( FLASH_CONFIG_DATA_START_ADDR + FLASH_PAGE_SIZE )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NRF_FSTORAGE_DEF(nrf_fstorage_t g_app_fstorage) =
{
    .evt_handler = NULL,
    .start_addr  = FLASH_CONFIG_DATA_START_ADDR,
    .end_addr    = FLASH_CONFIG_DATA_END_ADDR
};

static uint8_t g_is_initialized = 0;

static uint32_t g_active_page_addr;
static uint32_t g_swap_page_addr;

FlashConfigData_t g_persistent_conf;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ret_code_t FlashConfigData_Init(void)
{
    ret_code_t err_code;

    // Initialize the flash storage module
    err_code = nrf_fstorage_init(&g_app_fstorage, &nrf_fstorage_nvmc, NULL);
    require_noerr(err_code, exit);

    /*
     * Read from both pages to determine which one is
     * active and which one is swap...
     */
    FlashConfigData_t page1, page2;

    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  FLASH_CONFIG_DATA_PAGE1_ADDR,
                                  &page1,
                                  sizeof(FlashConfigData_t) );
    require_noerr(err_code, exit);

    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  FLASH_CONFIG_DATA_PAGE2_ADDR,
                                  &page2,
                                  sizeof(FlashConfigData_t) );
    require_noerr(err_code, exit);

    FlashConfigData_t *p_active_page;
    if (page1.swap_count > page2.swap_count)
    {
        g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        p_active_page      = &page1;
    }
    else if (page2.swap_count > page1.swap_count)
    {
        g_active_page_addr = FLASH_CONFIG_DATA_PAGE2_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE1_ADDR;
        p_active_page      = &page2;
    }
    else
    {
        // Both pages have to same swap count...
        GL_LOG("WARNING: Both pages have the same swap count\n");
        GL_LOG("defaulting to page 1 as the active page and page2 as swap...\n");

        g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        p_active_page      = &page1;
    }

    // memcpy the active page into the global config
    memcpy( &g_persistent_conf, p_active_page, sizeof(FlashConfigData_t) );

    // Mark as initialized
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
    GL_LOG("  - swap_count:             %d\n", g_persistent_conf.swap_count);
    GL_LOG("  - fw_update_pending:      %d\n", g_persistent_conf.fw_update_pending);
    GL_LOG("  - anchor_id:              %d\n", g_persistent_conf.anchor_id);
    GL_LOG("  - socket_recv_timeout_ms: %d\n", g_persistent_conf.swap_count);

    // Pointer to array containing the mac address
    uint8_t *p_mac_addr = g_persistent_conf.mac_addr;

    GL_LOG("  - mac_addr:               %02X:%02X:%02X:%02X:%02X:%02X\n",
                p_mac_addr[0], p_mac_addr[1], p_mac_addr[2],
                p_mac_addr[3], p_mac_addr[4], p_mac_addr[5]  );

    GL_LOG("  - using_dhcp:             %d\n", g_persistent_conf.using_dhcp);
}

ret_code_t FlashConfigData_WriteBack(void)
{
    ret_code_t err_code;

    // Increment the swap count
    g_persistent_conf.swap_count++;

    // Write the config data to the swap page
    err_code = nrf_fstorage_write( &g_app_fstorage,
                                   g_swap_page_addr,
                                   &g_persistent_conf,
                                   sizeof(g_persistent_conf),
                                   NULL );
    require_noerr(err_code, exit);

    // Swap the active and swap page addresses
    uint32_t tmp       = g_active_page_addr;
    g_active_page_addr = g_swap_page_addr;
    g_swap_page_addr   = tmp;

    GL_LOG("Successfully wrote config data to flash!\n");
    FlashConfigData_Print();
    GL_LOG("\n");

exit:
    return err_code;
}

ret_code_t FlashConfigData_RestoreFromSwap(void)
{
    ret_code_t err_code;

    uint32_t swap_count = g_persistent_conf.swap_count;

    // Read the config data from the swap page
    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  g_swap_page_addr,
                                  &g_persistent_conf,
                                  sizeof(g_persistent_conf) );
    require_noerr(err_code, exit);

    /*
     * Take the max so that we are sure that this write is the
     * one that will be used in the future
     */
    g_persistent_conf.swap_count = MAX(swap_count, g_persistent_conf.swap_count);

    // Write the config data to the active page
    err_code = FlashConfigData_WriteBack();
    require_noerr(err_code, exit);

exit:
    return err_code;
}
