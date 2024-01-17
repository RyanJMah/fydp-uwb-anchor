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

#define RECV_BUF_SIZE                   ( 1024*2 )

#define MDNS_PORT                       ( 5353 )
#define MDNS_ADDR                       { 224, 0, 0, 251 }
#define MULTICAST_MAC_ADDR              { 0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb }

// in microseconds
#define TWO_SECONDS                     ( 2000000 )

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

static __attribute__((aligned(4))) uint8_t g_recv_buf[RECV_BUF_SIZE];

static wiz_NetInfo g_net_info;

#if NEEDS_MUTEX
static osMutexDef(g_lan_mutex);     // Declare mutex
static osMutexId (g_lan_mutex_id);  // Mutex ID
#endif

static uint8_t g_mdns_addr[] = MDNS_ADDR;

static uint8_t g_hardcoded_mdns_pkt[] =
{
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
};
static uint8_t g_multicast_mac_addr[] = MULTICAST_MAC_ADDR;


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

static void _timeout_timer_start(void)
{
    // Configure Timer1
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;

    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;  // Set Timer mode
    NRF_TIMER1->PRESCALER = 4;  // Set prescaler (16MHz / 2^4 = 1MHz)
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_32Bit;  // Set 32-bit mode

    NRF_TIMER1->INTENCLR = (0x3F << 16U);  /* disabling all interupts for Timer 1*/

    // Start the timer
    NRF_TIMER1->TASKS_START = 1;
}

static void _timeout_timer_stop(void)
{
    NRF_TIMER1->TASKS_STOP  = 1;
    NRF_TIMER1->TASKS_CLEAR = 1;
}

static uint32_t _timeout_timer_get_us(void)
{
    NRF_TIMER1->TASKS_CAPTURE[0] = 1;
    return NRF_TIMER1->CC[0];
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


    _timeout_timer_start();

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

        if ( _timeout_timer_get_us() > TWO_SECONDS )
        {
            GL_LOG("DHCP TIMEOUT, retrying...\r\n");

            // Re-initialize DHCP
            DHCP_init( DHCP_SOCK_NUM, g_recv_buf );

            // restart timer
            _timeout_timer_stop();
            _timeout_timer_start();

            continue;
        }
    }

    _timeout_timer_stop();

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

/*
 * FIXME: Doesn't actually use the hostname parameter, we probably arne't going to
 *        ever change the hostname tho, so probably fine
 */
int16_t LAN_GetServerIPViaMDNS( hostname_t __attribute__((unused)) hostname,
                                ipv4_addr_t* out_addr )
{
    int16_t err_code;

    #if NEEDS_MUTEX
    bool mutex_taken = _take_mutex();
    #endif

    GL_LOG("creating mdns socket...\n");

    // Needed for multicast sockets, set the multicast addr and port
    setSn_DIPR(MDNS_SOCK_NUM, g_mdns_addr);
    setSn_DPORT(MDNS_SOCK_NUM, MDNS_PORT);

    err_code = socket( MDNS_SOCK_NUM, Sn_MR_UDP, MDNS_PORT, SF_MULTI_ENABLE );
    require( err_code == MDNS_SOCK_NUM, exit );

    GL_LOG("sending mDNS packet...\n");


    err_code = sendto_mac( MDNS_SOCK_NUM,
                            g_hardcoded_mdns_pkt,
                            sizeof(g_hardcoded_mdns_pkt),
                            g_multicast_mac_addr,
                            g_mdns_addr,
                            MDNS_PORT );
    require( err_code > 0, exit );

    GL_LOG("waiting for response...\n");

    uint8_t  dst_ip[4];
    uint16_t dst_port;

    _timeout_timer_start();

    while ( getSn_RX_RSR(MDNS_SOCK_NUM) == 0 )  // wait for response
    {
        if ( _timeout_timer_get_us() > TWO_SECONDS )
        {
            GL_LOG("mDNS TIMEOUT\n");

            err_code = SOCKERR_TIMEOUT;
            goto exit;
        }
    }

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


