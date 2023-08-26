#include <stdbool.h>
#include "macros.h"
#include "custom_board.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "user_ethernet.h"
#include "user_spi.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"
#include "gl_log.h"
#include "anchor_config.h"
#include "lan.h"

#ifdef GL_BOOTLOADER
    #define PRIO_GPIOTE_IRQn    ( 0 )
#else
    #include "cmsis_os.h"
    #include "int_priority.h"
#endif


/*************************************************************
 * MACROS
 ************************************************************/
// Bootloader is baremetal, so doesn't need mutex
#define NEEDS_MUTEX             ( !defined(GL_BOOTLOADER) )

#if NEEDS_MUTEX
#define MUTEX_WAIT_TIMEOUT_MS   ( 500 )
#endif

#define DHCP_BUF_SIZE           ( 1024*2 )

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

static uint8_t g_dhcp_buf[DHCP_BUF_SIZE];

static wiz_NetInfo g_net_info =
{
    .mac = ANCHOR_MAC_ADDR,
    .sn = ANCHOR_LAN_SUBNET_MASK,
    .gw   = ANCHOR_LAN_GW_ADDR,

#if !ANCHOR_LAN_USING_DHCP
    .ip = ANCHOR_LAN_IP_ADDR,
#endif

#if ANCHOR_USE_DHCP
    .dhcp = NETINFO_DHCP
#else
    .dhcp = NETINFO_STATIC
#endif
};

#if NEEDS_MUTEX
static osMutexDef(g_lan_mutex);     // Declare mutex
static osMutexId (g_lan_mutex_id);  // Mutex ID
#endif

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
#if NEEDS_MUTEX
static ALWAYS_INLINE bool _take_mutex(void)
{
    return osMutexWait(g_lan_mutex_id, MUTEX_WAIT_TIMEOUT_MS) == osOK;
}

static ALWAYS_INLINE void _release_mutex(void)
{
    osMutexRelease(g_lan_mutex_id);
}
#endif

static ALWAYS_INLINE void _init_reset_pin(void)
{
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

    if ( nrf_drv_gpiote_out_init( W5500_RESET_PIN,
                                  &out_config) != NRF_SUCCESS )
    {
        GL_LOG("FAILED TO INIT RESET PIN\n");
    }
}

static ALWAYS_INLINE void _reset_via_reset_pin(void)
{
    nrf_drv_gpiote_out_clear(W5500_RESET_PIN);
    nrf_delay_ms(100);
    nrf_drv_gpiote_out_set(W5500_RESET_PIN);
    nrf_delay_ms(1000);
}

static ALWAYS_INLINE void _deinit_reset_pin(void)
{
    nrf_drv_gpiote_out_uninit(W5500_RESET_PIN);
}

static ALWAYS_INLINE void _init_interrupts(nrfx_gpiote_evt_handler_t isr_func)
{
    // set interrupt priority
    NVIC_SetPriority(GPIOTE_IRQn, PRIO_GPIOTE_IRQn);

    // configure interrupt on falling edge
    nrf_drv_gpiote_in_config_t int_pin_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    if ( nrf_drv_gpiote_in_init( W5500_INTERRUPT_PIN,
                                 &int_pin_config,
                                 isr_func ) != NRF_SUCCESS )
    {
        GL_LOG("FAILED TO INITIALIZE W5500 INTERRUPT PIN\n");
    }
    nrf_drv_gpiote_in_event_enable(W5500_INTERRUPT_PIN, true);

    // disable global interrupts...
    setIMR(0b1111 << 4);
    if ( getIMR() != (0b1111 << 4) )
    {
        GL_LOG("FAILED TO DISABLE GLOBAL INTERRUPTS ON W5500\n");
    }

    // configure interrupts on MQTT socket
    setSIMR(1 << MQTT_SOCK_NUM);
    if ( getSIMR() != (1 << MQTT_SOCK_NUM) )
    {
        GL_LOG("FAILED TO ENABLE SOCKET INTERRUPT ON W5500\n");
    }

    // Enable recv interrupt
    setSn_IMR(MQTT_SOCK_NUM, 1 << 2);
    if ( getSn_IMR(MQTT_SOCK_NUM) != 1 << 2 )
    {
        GL_LOG("FAILED TO ENABLE RECT SOCKET INTERRUPT ON W5500\n");
    }

    nrf_delay_ms(100);
}

static void _print_net_info(void)
{
    memset( &g_net_info, 0, sizeof(g_net_info) );
    wizchip_getnetinfo( &g_net_info );

    GL_LOG("\r\nNETWORK CONFIGURATION:\r\n");
    GL_LOG("======================\r\n");

    GL_LOG( "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
            g_net_info.mac[0], g_net_info.mac[1], g_net_info.mac[2],
            g_net_info.mac[3], g_net_info.mac[4], g_net_info.mac[5] );

    GL_LOG( "IP Address:  %d.%d.%d.%d\r\n",
            g_net_info.ip[0], g_net_info.ip[1], g_net_info.ip[2], g_net_info.ip[3] );

    GL_LOG( "Gateway:     %d.%d.%d.%d\r\n",
            g_net_info.gw[0], g_net_info.gw[1], g_net_info.gw[2], g_net_info.gw[3] );

    GL_LOG( "Subnet Mask: %d.%d.%d.%d\r\n",
            g_net_info.sn[0], g_net_info.sn[1], g_net_info.sn[2], g_net_info.sn[3] );

    GL_LOG( "DNS Server:  %d.%d.%d.%d\r\n",
            g_net_info.dns[0], g_net_info.dns[1], g_net_info.dns[2], g_net_info.dns[3]);

    GL_LOG("======================\r\n\n");
}

static void _network_init(void)
{
    ctlnetwork(CN_SET_NETINFO, (void*)&g_net_info);

    _print_net_info();
}

static ALWAYS_INLINE void _get_ip_via_dhcp(void)
{
    DHCP_init( DHCP_SOCK_NUM, g_dhcp_buf );
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

void LAN_Init(nrfx_gpiote_evt_handler_t isr_func)
{
    GL_LOG("Initializing ethernet...\n");

    #if NEEDS_MUTEX
    g_lan_mutex_id = osMutexCreate( osMutex(g_lan_mutex) );
    #endif

    // Make sure GPIO is initialized
    nrf_drv_gpiote_init();

    // Reset the chip...
    _init_reset_pin();
    _reset_via_reset_pin();
    _deinit_reset_pin();

    spi1_master_init();
    user_ethernet_init();

    _network_init();

    _init_interrupts( isr_func );

    GL_LOG("Ethernet initialized!\n");
}

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port)
{
    int16_t err_code;

    #if NEEDS_MUTEX
    bool mutex_taken = _take_mutex();
    #endif

    GL_LOG("creating socket...\n");

    err_code = socket( sock, Sn_MR_TCP, g_internal_port++, 0 );
    require( err_code > 0, exit );

    GL_LOG(
            "connecting to server hosted at %d.%d.%d.%d\n",
            addr.bytes[0],
            addr.bytes[1],
            addr.bytes[2],
            addr.bytes[3] );

    err_code = connect(sock, addr.bytes, port);
    require( err_code > 0, exit );

exit:
    #if NEEDS_MUTEX
    if ( mutex_taken )
    {
        _release_mutex();
    }
    #endif

    return err_code;
}

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len)
{
    #if NEEDS_MUTEX
    bool mutex_taken = _take_mutex();
    #endif

    int32_t ret = send(sock, data, len);

    #if NEEDS_MUTEX
    if ( mutex_taken )
    {
        _release_mutex();
    }
    #endif

    return ret;
}

int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len)
{
    #if NEEDS_MUTEX
    bool mutex_taken = _take_mutex();
    #endif

    int32_t ret = recv(sock, out_data, len);

    #if NEEDS_MUTEX
    if ( mutex_taken )
    {
        _release_mutex();
    }
    #endif

    return ret;
}
