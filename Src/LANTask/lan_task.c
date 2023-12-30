#include "cmsis_os.h"
#include "flash_config_data.h"
#include "fira_helper.h"
#include "gl_error.h"
#include "macros.h"
#include "gl_log.h"
#include "custom_board.h"
#include "w5500.h"
#include "nrf_delay.h"
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
    // GL_LOG("GOT W5500 INTERRUPT\n");
    // GL_LOG("sir=%d\n", getSIR());

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

    MqttClient_Publish(g_HEARTBEAT_TOPIC, _heartbeat_json, _heartbeat_json_len);
}

static ALWAYS_INLINE void _clear_interrupts(void)
{
    uint8_t sir = getSIR();

    if ( sir & (1 << MQTT_SOCK_NUM) )
    {
        setSn_IR(MQTT_SOCK_NUM, 1 << 2);
    }
}

static ALWAYS_INLINE void _init_mqtt(void)
{
    GL_LOG("INITIALIZING MQTT AND LAN...\n");

    // Inits topic strings based on anchor id
    AppMqtt_Init();

    // Initializes the W5500
    LAN_Init( interrupt_pin_handler );

    // Initialize MQTT
    MqttRetCode_t err_code = MqttClient_Init();

    if ( err_code != MQTT_OK )
    {
        GL_LOG("FAILED TO CONNECT TO BROKER, err_code=%d\n", err_code);
        GL_FATAL_ERROR();
    }

    // Create and start the heartbeat timer...
    g_heartbeat_timer_id = osTimerCreate( osTimer(g_heartbeat_timer),
                                          osTimerPeriodic,
                                          NULL );
    osTimerStart( g_heartbeat_timer_id, MQTT_HEARTBEAT_PERIODICITY_MS );
}

static void _LANTask_Main(void const* args UNUSED)
{
    MqttRetCode_t err_code;

    _init_mqtt();

    while (1)
    {
        osSignalWait(RECV_INTERRUPT_SIGNAL, osWaitForever);

        // GL_LOG("RECEIVED MESSAGE!");

        err_code = MqttClient_ManageRunLoop();
        switch (err_code)
        {
            case MQTT_NO_MEM:
            case MQTT_SERVER_REFUSED:
            case MQTT_ILLEGAL_STATE:
            case MQTT_SOCK_INTERNAL_ERR:
            {
                GL_LOG("ManageRunLoop failed: err_code=%d\n", err_code);
                GL_FATAL_ERROR();
            }

            default:
            {
                break;
            }
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
        GL_LOG("FAILED TO CREATE LAN TASK...\n");
    }
    //////////////////////////////////////////////////////////////////////////
}
