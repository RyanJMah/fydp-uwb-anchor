#include <stdbool.h>
#include "cmsis_os.h"
#include "macros.h"
#include "int_priority.h"
#include "custom_board.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "user_ethernet.h"
#include "user_spi.h"
#include "w5500.h"
#include "socket.h"
#include "gl_log.h"
#include "lan.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define MUTEX_WAIT_TIMEOUT_MS   ( 500 )

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

static osMutexDef(g_lan_mutex);     // Declare mutex
static osMutexId (g_lan_mutex_id);  // Mutex ID

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static ALWAYS_INLINE bool _take_mutex(void)
{
    return osMutexWait(g_lan_mutex_id, MUTEX_WAIT_TIMEOUT_MS) == osOK;
}

static ALWAYS_INLINE void _release_mutex(void)
{
    osMutexRelease(g_lan_mutex_id);
}

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

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

void LAN_Init(nrfx_gpiote_evt_handler_t isr_func)
{
    GL_LOG("Initializing ethernet...\n");

    g_lan_mutex_id = osMutexCreate( osMutex(g_lan_mutex) );

    // Make sure GPIO is initialized
    nrf_drv_gpiote_init();

    // Reset the chip...
    _init_reset_pin();
    _reset_via_reset_pin();
    _deinit_reset_pin();

    spi1_master_init();
    user_ethernet_init();

    _init_interrupts( isr_func );

    GL_LOG("Ethernet initialized!\n");
}

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port)
{
    int16_t err_code;
    bool    mutex_taken = _take_mutex();

    GL_LOG("creating socket...\n");

    err_code = socket( sock, Sn_MR_TCP, g_internal_port++, 0 );
    require( err_code == SOCK_OK, exit );

    GL_LOG(
            "connecting to server hosted at %d.%d.%d.%d\n",
            addr.bytes[0],
            addr.bytes[1],
            addr.bytes[2],
            addr.bytes[3] );

    err_code = connect(sock, addr.bytes, port);
    require( err_code == SOCK_OK, exit );

exit:
    if ( mutex_taken )
    {
        _release_mutex();
    }
    return err_code;
}

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len)
{
    bool mutex_taken = _take_mutex();

    int32_t ret = send(sock, data, len);

    if ( mutex_taken )
    {
        _release_mutex();
    }
    return ret;
}

int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len)
{
    bool mutex_taken = _take_mutex();

    int32_t ret = recv(sock, out_data, len);

    if ( mutex_taken )
    {
        _release_mutex();
    }
    return ret;
}
