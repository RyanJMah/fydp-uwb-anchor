#include "nrf_error.h"
#include "nrf_fstorage.h"
#include "nrf_log_ctrl.h"
#include "crc32.h"

#ifdef GL_BOOTLOADER
    #include "nrf_fstorage_nvmc.h"
#else
    #include "nrf_fstorage_sd.h"
#endif

#include "Protobufs/AutoGenerated/C/flash_config_data.pb.h"
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

/*
 * What will be in flash looks like this:
 *
 * --------------------------------------------------
 * | FlashConfigData (Serialized) | CRC32 | Padding |
 * --------------------------------------------------
 */

// Size of serialized FlashConfigData + 4 bytes of CRC32
#define PB_BUFF_LEN                     ( FlashConfigData_size + 4 )
#define PB_BUFF_NUM_PADDING_BYTES       ( 4 - (PB_BUFF_LEN % 4) )

#if (PB_BUFF_LEN + PB_BUFF_NUM_PADDING_BYTES) > FLASH_PAGE_SIZE
    #error "FlashConfigData won't fit in a page!"
#endif

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef enum
{
    PAGE_1 = 0,
    PAGE_2
} FlashConfig_Page_t;

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

/*
 * Yes, I know this technically reads as "protocol buffer buffer", but I
 * think it's the most readable way to name this variable.
 */
static uint8_t g_pb_buff[PB_BUFF_LEN + PB_BUFF_NUM_PADDING_BYTES];

static uint32_t g_active_page_crc;

FlashConfigData g_persistent_conf = FlashConfigData_init_zero;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static ALWAYS_INLINE uint32_t _compute_crc_for_global_buf()
{
    return crc32_compute( g_pb_buff, PB_BUFF_LEN, NULL );
}

static ALWAYS_INLINE bool _check_crc_for_global_buf( uint32_t expected )
{
    return expected == _compute_crc_for_global_buf();
}

static ret_code_t _pb_decode_from_flash_to_global_buf( FlashConfig_Page_t page,
                                                       uint32_t* out_page_crc )
{
    ret_code_t err_code;
    bool pb_status;

    uint32_t addr;
    switch (page)
    {
        case PAGE_1:
        {
            addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
            break;
        }

        case PAGE_2:
        {
            addr = FLASH_CONFIG_DATA_PAGE2_ADDR;
            break;
        }

        default:
        {
            return NRF_ERROR_INVALID_PARAM;
        }
    }

    // Read from flash
    err_code = nrf_fstorage_read( &g_app_fstorage,
                                  addr,
                                  g_pb_buff,
                                  sizeof(g_pb_buff) );
    require_noerr(err_code, exit);

    // Protobuf input stream
    pb_istream_t stream = pb_istream_from_buffer( g_pb_buff, sizeof(g_pb_buff) );

    // Decode the config data from the page
    pb_status = pb_decode( &stream, FlashConfigData_fields, &g_persistent_conf );
    require_action(pb_status == 1, exit, err_code = NRF_ERROR_INTERNAL);

    if ( out_page_crc != NULL )
    {
        *out_page_crc = ( g_pb_buff[FlashConfigData_size + 0] << (8*0) ) |
                        ( g_pb_buff[FlashConfigData_size + 1] << (8*1) ) |
                        ( g_pb_buff[FlashConfigData_size + 2] << (8*2) ) |
                        ( g_pb_buff[FlashConfigData_size + 3] << (8*3) );
    }

exit:
    return err_code;
}

static ret_code_t _pb_encode_to_global_buf(uint32_t* out_bytes_written)
{
    // Protobuf output stream
    pb_ostream_t stream = pb_ostream_from_buffer( g_pb_buff, sizeof(g_pb_buff) );

    // Encode the config data to the page
    bool status = pb_encode( &stream, FlashConfigData_fields, &g_persistent_conf );

    if ( out_bytes_written != NULL )
    {
        *out_bytes_written = stream.bytes_written;
    }

    return (status) ? NRF_SUCCESS : NRF_ERROR_INTERNAL;
}

static void _append_crc_to_global_buf(uint32_t crc)
{
    g_pb_buff[FlashConfigData_size + 0] = (crc >> (8*0)) & 0xFF;
    g_pb_buff[FlashConfigData_size + 1] = (crc >> (8*1)) & 0xFF;
    g_pb_buff[FlashConfigData_size + 2] = (crc >> (8*2)) & 0xFF;
    g_pb_buff[FlashConfigData_size + 3] = (crc >> (8*3)) & 0xFF;
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

// Use SD for app, NVMC for bootloader
#ifdef GL_BOOTLOADER
    err_code = nrf_fstorage_init(&g_app_fstorage, &nrf_fstorage_nvmc, NULL);
    require_noerr(err_code, exit);
#else
    err_code = nrf_fstorage_init(&g_app_fstorage, &nrf_fstorage_sd, NULL);
    require_noerr(err_code, exit);
#endif

    uint32_t page1_swap_count, page2_swap_count;
    uint32_t page1_crc, page2_crc;
    bool     page1_crc_good, page2_crc_good;

    // Read from page1 first
    err_code = _pb_decode_from_flash_to_global_buf( PAGE_1, &page1_crc );
    require_noerr(err_code, exit);

    page1_crc_good   = _check_crc_for_global_buf( page1_crc );
    page1_swap_count = g_persistent_conf.swap_count;

    // Read from page2
    err_code = _pb_decode_from_flash_to_global_buf( PAGE_2 , &page2_crc );
    require_noerr(err_code, exit);

    page2_crc_good   = _check_crc_for_global_buf( page2_crc );
    page2_swap_count = g_persistent_conf.swap_count;

    // Select which page should be active and which should be swap
    FlashConfig_Page_t active_page;

    if ( page1_crc_good && page2_crc_good )
    {
        // If both page's CRC is good, select the higher swap count
        GL_LOG("Both pages CRC is valid, selecting the page with the highest swap count...\n");

        if ( page1_swap_count > page2_swap_count )
        {
            GL_LOG("Selecting page1 as the active page and page2 as swap...\n");

            active_page        = PAGE_1;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        }
        else if ( page2_swap_count > page1_swap_count )
        {
            GL_LOG("Selecting page2 as the active page and page1 as swap...\n");

            active_page        = PAGE_2;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE2_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE1_ADDR;
        }
        else
        {
            // Both pages have to same swap count...
            GL_LOG("WARNING: Both pages have the same swap count\n");
            GL_LOG("defaulting to page 1 as the active page and page2 as swap...\n");

            active_page        = PAGE_1;
            g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
            g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
        }
    }
    else if ( page1_crc_good && !page2_crc_good )
    {
        // If only page1 has good CRC, select page1 as active
        GL_LOG("WARNING: page2 CRC is invalid, selecting page1 as active...\n");

        active_page        = PAGE_1;
        g_active_page_addr = FLASH_CONFIG_DATA_PAGE1_ADDR;
        g_swap_page_addr   = FLASH_CONFIG_DATA_PAGE2_ADDR;
    }
    else if ( !page1_crc_good && page2_crc_good )
    {
        // If only page2 has good CRC, select page2 as active
        GL_LOG("WARNING: page1 CRC is invalid, selecting page2 as active...\n");

        active_page        = PAGE_2;
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

    // Read from the active page
    err_code = _pb_decode_from_flash_to_global_buf( active_page, &g_active_page_crc );
    require_noerr(err_code, exit);

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
}

ret_code_t FlashConfigData_WriteBack(void)
{
    if ( !g_is_initialized )
    {
        return NRF_ERROR_INVALID_STATE;
    }

    ret_code_t err_code;
    uint32_t bytes_written;

    // Increment the swap count
    g_persistent_conf.swap_count++;


    // Encode the config data
    err_code = _pb_encode_to_global_buf( &bytes_written );
    require_noerr(err_code, exit);

    // Compute the CRC of the serialized data
    g_active_page_crc = _compute_crc_for_global_buf();

    _append_crc_to_global_buf( g_active_page_crc );
    bytes_written += 4;


    // Write needs to be aligned to 4 bytes, calculate number of bytes to pad
    uint8_t padding = 4 - (bytes_written % 4);

    uint32_t num_bytes_to_write = bytes_written + padding;
    require_action( num_bytes_to_write <= FLASH_PAGE_SIZE,
                    exit,
                    err_code = NRF_ERROR_INTERNAL );

    memset( &g_pb_buff[bytes_written], 0xFF, padding );


    // Erase the swap page before writing to it
    err_code = nrf_fstorage_erase( &g_app_fstorage,
                                   g_swap_page_addr,
                                   1,
                                   NULL );
    require_noerr(err_code, exit);

    // Write to swap page
    err_code = nrf_fstorage_write( &g_app_fstorage,
                                   g_active_page_addr,
                                   g_pb_buff,
                                   num_bytes_to_write,
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
