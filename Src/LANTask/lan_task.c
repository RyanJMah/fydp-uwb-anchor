#include "cmsis_os.h"
#include "anchor_config.h"
#include "fira_helper.h"
#include "macros.h"
#include "deca_dbg.h"
#include "custom_board.h"
#include "w5500.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "app_mqtt.h"
#include "mqtt_client.h"
#include "lan.h"
#include "lan_task.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_STACK_SIZE     ( 1024U*2 )

#define RECV_INTERRUPT_SIGNAL   ( 0x01 )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static osThreadId g_lan_task_id;
static osTimerId  g_heartbeat_timer_id;

// Foward declare timer callback...
static void _send_heartbeat(const void* args UNUSED);
static osTimerDef(g_heartbeat_timer, _send_heartbeat);


/*************************************************************
 * INTERRUPT HANDLERS
 ************************************************************/
static void interrupt_pin_handler( nrf_drv_gpiote_pin_t pin,
                                   nrf_gpiote_polarity_t action )
{
    // diag_printf("GOT W5500 INTERRUPT\n");
    // diag_printf("sir=%d\n", getSIR());

    if ( action != GPIOTE_CONFIG_POLARITY_HiToLo )
    {
        return;
    }

    if ( pin != W5500_INTERRUPT_PIN )
    {
        return;
    }

    // Signal mainloop to processes received msg
    osSignalSet(g_lan_task_id, RECV_INTERRUPT_SIGNAL);
}

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static void _send_heartbeat(const void* args UNUSED)
{
    static char _heartbeat_json[]      = "{\"status\": \"online\"}";
    static uint8_t _heartbeat_json_len = sizeof(_heartbeat_json) - 1;

    MqttClient_Publish(HEARTBEAT_TOPIC, _heartbeat_json, _heartbeat_json_len);
}

static ALWAYS_INLINE void _clear_interrupts(void)
{
    uint8_t sir = getSIR();

    if ( sir & (1 << MQTT_SOCK_NUM) )
    {
        setSn_IR(MQTT_SOCK_NUM, 1 << 2);
    }
}

static void _LANTask_Main(void const* args UNUSED)
{
    MqttRetCode_t err_code;

    diag_printf("INITIALIZING MQTT AND LAN...\n");

    // Initializes the W5500
    LAN_Init( interrupt_pin_handler );

    // Initialize MQTT
    err_code = MqttClient_Init();
    if ( err_code != MQTT_OK )
    {
        diag_printf("FAILED TO CONNECT TO BROKER, err_code=%d\n", err_code);
    }

    // Create and start the heartbeat timer...
    g_heartbeat_timer_id = osTimerCreate( osTimer(g_heartbeat_timer),
                                          osTimerPeriodic,
                                          NULL );
    osTimerStart( g_heartbeat_timer_id, MQTT_HEARTBEAT_PERIODICITY_MS );

    while (1)
    {
        osSignalWait(RECV_INTERRUPT_SIGNAL, osWaitForever);

        // diag_printf("RECEIVED MESSAGE!");

        err_code = MqttClient_ManageRunLoop();
        if ( (err_code != MQTT_OK) && (err_code != MQTT_NEED_MORE_BYTES) )
        {
            diag_printf("ManageRunLoop failed: err_code=%d\n", err_code);
        }

        _clear_interrupts();
    }
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void)
{
    //////////////////////////////////////////////////////////////////////////
    // Initialize task
    osThreadDef(LANTask, _LANTask_Main, LAN_TASK_PRIORITY, 0, LAN_TASK_STACK_SIZE);
    g_lan_task_id = osThreadCreate( osThread(LANTask), NULL );

    if ( g_lan_task_id == NULL )
    {
        diag_printf("FAILED TO CREATE LAN TASK...\n");
    }
    //////////////////////////////////////////////////////////////////////////
}
