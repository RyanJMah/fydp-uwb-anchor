#include <stdbool.h>
#include "macros.h"
#include "custom_board.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "user_ethernet.h"
#include "user_spi.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "socket.h"
#include "dhcp.h"
#include "gl_log.h"
#include "gl_error.h"
#include "flash_config_data.h"
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
#define NEEDS_MUTEX                     ( !defined(GL_BOOTLOADER) )

#if NEEDS_MUTEX
#define MUTEX_WAIT_TIMEOUT_MS           ( 500 )
#endif

// timeout value = 10ms
#define DEFAULT_TIMEOUT_RETRY_CNT       ( 5 )    
#define DEFAULT_TIMEOUT_TIME_100US      ( 1000 )	

#define RECV_BUF_SIZE                   ( 1024*2 )

#define MDNS_PORT                       ( 5353 )
#define MDNS_ADDR                       { 224, 0, 0, 251 }

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

static __attribute__((aligned(4))) uint8_t g_recv_buf[RECV_BUF_SIZE];

static wiz_NetInfo g_net_info;

#ifndef GL_BOOTLOADER
APP_TIMER_DEF(g_dhcp_timer);
#endif

#if NEEDS_MUTEX
static osMutexDef(g_lan_mutex);     // Declare mutex
static osMutexId (g_lan_mutex_id);  // Mutex ID
#endif

static uint8_t g_mdns_addr[] = MDNS_ADDR;

static uint8_t g_hardcoded_mdns_pkt[] = {
    0x01, 0x02,
    0x00, 0x00,
    0x00, 0x01,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x0c, 'G', 'u', 'i', 'd', 'i', 'n', 'g', 'L', 'i', 'g', 'h', 't',
    0x05, '_', 'm', 'q', 't', 't',
    0x04, '_', 't', 'c', 'p',
    0x05, 'l', 'o', 'c', 'a', 'l', 0x00,
    0x00, 0x01,
    0x00, 0x01,
    // 0x00
};

// static uint8_t g_hardcoded_mdns_pkt[] =
// {
//     0x60, 0xa6,
//     // 0x00, 0x00,
//     0x00, 0x00,
//     0x00, 0x01,
//     0x00, 0x00,
//     0x00, 0x00,
//     0x00, 0x00,
//     0x0c, 'G', 'u', 'i', 'd', 'i', 'n', 'g', 'L', 'i', 'g', 'h', 't',
//     0x05, '_', 'm', 'q', 't', 't',
//     0x04, '_', 't', 'c', 'p',
//     0x05, 'l', 'o', 'c', 'a', 'l', '\0',
//     0x00, 0x01,
//     0x00, 0x01,
//     // 0x00
// };

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

static inline void _set_timeout(uint8_t retry_cnt, uint16_t time_100us)
{
    wiz_NetTimeout timeout_info =
    {
        .retry_cnt = retry_cnt,
        .time_100us = time_100us
    };
    wizchip_settimeout(&timeout_info);

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

static ALWAYS_INLINE void _print_net_info(void)
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

static ALWAYS_INLINE void _static_net_init(void)
{
    memset( &g_net_info, 0, sizeof(g_net_info) );

    memcpy( g_net_info.mac, gp_persistent_conf->mac_addr, sizeof(g_net_info.mac) );

    g_net_info.dhcp = NETINFO_STATIC;

    memcpy( g_net_info.ip,
            gp_persistent_conf->static_ip_addr.bytes,
            sizeof(g_net_info.ip) );

    if ( IPAddr_IsInvalid(gp_persistent_conf->static_gateway) )
    {
        memset( g_net_info.gw, 0, sizeof(g_net_info.gw) );
    }
    else
    {
        memcpy( g_net_info.gw,
                gp_persistent_conf->static_gateway.bytes,
                sizeof(g_net_info.gw) );
    }

    memcpy( g_net_info.sn,
            gp_persistent_conf->static_netmask.bytes,
            sizeof(g_net_info.sn) );

    ctlnetwork(CN_SET_NETINFO, (void*)&g_net_info);
}

#ifndef GL_BOOTLOADER
static void _DHCP_Time_Handler(void * p_context)
{
    DHCP_time_handler();
}
#endif

static ALWAYS_INLINE void _dhcp_net_init(void)
{
    int8_t ret_code = DHCP_FAILED;

    GL_LOG("Getting IP via DHCP...\n");

    // Initialize mac address from flash
    memset( &g_net_info, 0, sizeof(g_net_info) );
    memcpy( g_net_info.mac, gp_persistent_conf->mac_addr, sizeof(g_net_info.mac) );

    // Commit changes
    ctlnetwork(CN_SET_NETINFO, (void*)&g_net_info);

    // Initialize DHCP
    DHCP_init( DHCP_SOCK_NUM, g_recv_buf );

    // Initialize timer

#ifndef GL_BOOTLOADER
    ret_code_t err_code = NRF_SUCCESS;

    err_code = app_timer_init();
    GL_ERROR_CHECK(err_code);

    err_code = app_timer_create(&g_dhcp_timer, APP_TIMER_MODE_REPEATED, _DHCP_Time_Handler);
    GL_ERROR_CHECK(err_code);

    err_code = app_timer_start(g_dhcp_timer, APP_TIMER_TICKS(1000), NULL);
    GL_ERROR_CHECK(err_code);

    GL_LOG("DHCP INITIALIZED\n");

    while (1)
    {
        ret_code = DHCP_run();

        if ( ret_code == DHCP_IP_LEASED )
        {
            GL_LOG("DHCP IP ASSIGNED\r\n");
            break;
        }
        else if ( ret_code == DHCP_FAILED )
        {
            GL_LOG("DHCP FAILED, retrying in 5 seconds...\r\n");
            nrf_delay_ms(5000);
            continue;
        }
    }
#else
    // Can't get timers to work in bootloader for some reason, too lazy to debug,
    // will just use a lazier implementation

    nrf_delay_ms(6000);

    while (1)
    {
        ret_code = DHCP_run();

        if ( ret_code == DHCP_IP_LEASED )
        {
            GL_LOG("DHCP IP ASSIGNED\r\n");
            break;
        }
        else if ( ret_code == DHCP_FAILED )
        {
            GL_LOG("DHCP FAILED, retrying in 5 seconds...\r\n");
            nrf_delay_ms(5000);
            continue;
        }
    }
#endif

    memset( &g_net_info, 0, sizeof(g_net_info) );

    g_net_info.dhcp = NETINFO_DHCP;
    getIPfromDHCP(g_net_info.ip);
    getGWfromDHCP(g_net_info.gw);
    getSNfromDHCP(g_net_info.sn);
    getDNSfromDHCP(g_net_info.dns);

    memcpy( g_net_info.mac, gp_persistent_conf->mac_addr, sizeof(g_net_info.mac) );

    ctlnetwork(CN_SET_NETINFO, (void*)&g_net_info);

    /*
     * This is technically not how we should be doing DCHP, we should always
     * be running DHCP because the lease can expire, etc.
     *
     * In the application code, we can do this in the LAN task.
     *
     * In the bootloader, we are gonna just get our IP address then dip.
     */
    DHCP_stop();
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

    _set_timeout(DEFAULT_TIMEOUT_RETRY_CNT, DEFAULT_TIMEOUT_TIME_100US);

    if ( gp_persistent_conf->using_dhcp )
    {
        _dhcp_net_init();
    }
    else
    {
        _static_net_init();
    }

    if ( isr_func != NULL )
    {
        _init_interrupts( isr_func );
    }

    _print_net_info();

    GL_LOG("Ethernet initialized!\n");
}

int16_t LAN_GetServerIPViaMDNS(hostname_t hostname, ipv4_addr_t* out_addr)
{
    int16_t err_code;

    #if NEEDS_MUTEX
    bool mutex_taken = _take_mutex();
    #endif

    GL_LOG("creating mdns socket...\n");

    // Needed for multicast sockets, set the multicast addr and port
    setSn_DIPR(MDNS_SOCK_NUM, g_mdns_addr);
    setSn_DPORT(MDNS_SOCK_NUM, MDNS_PORT);

    err_code = socket( MDNS_SOCK_NUM, Sn_MR_UDP, MDNS_PORT, SF_MULTI_ENABLE | SF_IGMP_VER2 );
    require( err_code == MDNS_SOCK_NUM, exit );

    GL_LOG("sending mDNS packet...\n");


    err_code = sendto( MDNS_SOCK_NUM,
                       g_hardcoded_mdns_pkt,
                       sizeof(g_hardcoded_mdns_pkt),
                       g_mdns_addr,
                       MDNS_PORT );
    require( err_code > 0, exit );

    GL_LOG("waiting for response...\n");

    uint8_t  dst_ip[4];
    uint16_t dst_port;

    // THIS IS SO BAD
    // nrf_delay_ms(1000);

    // while ( getSn_RX_RSR(MDNS_SOCK_NUM) == 0 )
    // {
    //     // wait for response
    // }

    // if ( getSn_RX_RSR(MDNS_SOCK_NUM) == 0 )
    // {
    //     GL_LOG("no response received\n");

    //     err_code = SOCKERR_TIMEOUT;
    //     goto exit;
    // }

    err_code = recvfrom( MDNS_SOCK_NUM,
                         g_recv_buf,
                         RECV_BUF_SIZE,
                         &dst_ip[0],
                         &dst_port );
    require( err_code > 0, exit );

    memcpy( out_addr->bytes, dst_ip, sizeof(dst_ip) );

    err_code = close( MDNS_SOCK_NUM );
    require( err_code > 0, exit );

exit:
    #if NEEDS_MUTEX
    if ( mutex_taken )
    {
        _release_mutex();
    }
    #endif

    if ( err_code < 0 )
    {
        GL_LOG("sock_err_code = %d\n", err_code);
    }

    return err_code;
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
    require_action( err_code > 0, exit, GL_LOG("sock_err_code = %d\n", err_code) );

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


