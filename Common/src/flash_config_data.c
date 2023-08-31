#include "nrf_fstorage.h"
#include "nrf_log_ctrl.h"
#include "crc32.h"

#ifdef GL_BOOTLOADER
    #include "nrf_fstorage_nvmc.h"
#else
    #include "nrf_fstorage_sd.h"
#endif

#include "pb_encode.h"
#include "pb_decode.h"

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

FlashConfigData g_persistent_conf;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static uint32_t _compute_crc(FlashConfigData *p_data)
{
    // Compute the crc32 of the config data, excluding the crc32 field
    return crc32_compute( (uint8_t*)p_data,
                           sizeof(FlashConfigData) - sizeof(uint32_t),
                           NULL );
}

static uint8_t _verify_crc(FlashConfigData *p_data)
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

    /*
     * Read from both pages to determine which one is
     * active and which one is swap...
     */
    FlashConfigData page1, page2;

    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  FLASH_CONFIG_DATA_PAGE1_ADDR,
                                  &page1,
                                  sizeof(FlashConfigData) );
    require_noerr(err_code, exit);

    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  FLASH_CONFIG_DATA_PAGE2_ADDR,
                                  &page2,
                                  sizeof(FlashConfigData) );
    require_noerr(err_code, exit);

    // Verify CRC for both pages
    uint8_t page1_crc_good = _verify_crc(&page1);
    uint8_t page2_crc_good = _verify_crc(&page2);

    // Select which page should be active and which should be swap
    FlashConfigData *p_active_page;

    if ( page1_crc_good && page2_crc_good )
    {
        // If both page's CRC is good, select the higher swap count
        GL_LOG("Both pages CRC is valid, selecting the page with the highest swap count...\n");

        if (page1.swap_count > page2.swap_count)
        {
            GL_LOG("Selecting page1 as the active page and page2 as swap...\n");

            p_active_page      = &page1;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        }
        else if (page2.swap_count > page1.swap_count)
        {
            GL_LOG("Selecting page2 as the active page and page1 as swap...\n");

            p_active_page      = &page2;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE2_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE1_ADDR;
        }
        else
        {
            // Both pages have to same swap count...
            GL_LOG("WARNING: Both pages have the same swap count\n");
            GL_LOG("defaulting to page 1 as the active page and page2 as swap...\n");

            p_active_page      = &page1;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        }
    }
    else if ( page1_crc_good && !page2_crc_good )
    {
        // If only page1 has good CRC, select page1 as active
        GL_LOG("WARNING: page2 CRC is invalid, selecting page1 as active...\n");

        p_active_page      = &page1;
        g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
    }
    else if ( !page1_crc_good && page2_crc_good )
    {
        // If only page2 has good CRC, select page2 as active
        GL_LOG("WARNING: page1 CRC is invalid, selecting page2 as active...\n");

        p_active_page      = &page2;
        g_active_page_addr = FLASH_CONFIG_DATA_PAGE2_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE1_ADDR;
    }
    else
    {
        // If both pages have bad CRC, select page1 as active
        GL_LOG("ERROR: invalid CRC for both pages, CONFIG DATA IS CORRUPT!\n");
        GL_LOG("Please reprovision the device...\n");

        NRF_LOG_FINAL_FLUSH();

        while (1)
        {
            // Do nothing...
        }
    }

    // memcpy the active page into the global config
    memcpy( &g_persistent_conf, p_active_page, sizeof(FlashConfigData) );

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

    return _verify_crc(&g_persistent_conf);
}

void FlashConfigData_Print(void)
{
    GL_LOG("FlashConfigData: \n");
    GL_LOG("  - swap_count:             %d\n", g_persistent_conf.swap_count);
    GL_LOG("  - fw_update_pending:      %d\n", g_persistent_conf.fw_update_pending);
    GL_LOG("  - anchor_id:              %d\n", g_persistent_conf.anchor_id);
    GL_LOG("  - socket_recv_timeout_ms: %d\n", g_persistent_conf.socket_recv_timeout_ms);

    // Pointer to array containing the mac address
    uint8_t *p_mac_addr = g_persistent_conf.mac_addr;

    GL_LOG("  - mac_addr:               %02X:%02X:%02X:%02X:%02X:%02X\n",
                p_mac_addr[0], p_mac_addr[1], p_mac_addr[2],
                p_mac_addr[3], p_mac_addr[4], p_mac_addr[5]  );

    GL_LOG("  - using_dhcp:             %d\n", g_persistent_conf.using_dhcp);
    GL_LOG("  - crc32:                  %08X\n", g_persistent_conf.crc32);
}

ret_code_t FlashConfigData_WriteBack(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    // Increment the swap count
    g_persistent_conf.swap_count++;

    // Re-compute the CRC
    g_persistent_conf.crc32 = _compute_crc(&g_persistent_conf);

    // Erase the swap page before writing to it
    err_code = nrf_fstorage_erase( &g_app_fstorage,
                                   g_swap_page_addr,
                                   1,
                                   NULL );
    require_noerr(err_code, exit);

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

exit:
    return err_code;
}

ret_code_t FlashConfigData_RestoreFromSwap(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;

    uint32_t swap_count = g_persistent_conf.swap_count;

    // Read the config data from the swap page
    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  g_swap_page_addr,
                                  &g_persistent_conf,
                                  sizeof(FlashConfigData) );
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
