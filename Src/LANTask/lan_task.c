#include "cmsis_os.h"
#include "fira_helper.h"
#include "macros.h"
#include "deca_dbg.h"
#include "nrf_drv_gpiote.h"
#include "custom_board.h"
#include "w5500.h"
#include "mqtt_client.h"
#include "lan.h"
#include "lan_task.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_STACK_SIZE     ( 1024U*2 )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static osThreadId      g_lan_task_id;

/*************************************************************
 * INTERRUPT HANDLERS
 ************************************************************/
static void int_pin_handler( nrf_drv_gpiote_pin_t pin,
                             nrf_gpiote_polarity_t action )
{
    diag_printf("GOT W5500 INTERRUPT");
}

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static void _LANTask_Main(void const* args UNUSED)
{
    // MqttRetCode_t err_code;

    while (1)
    {
        // err_code = MqttClient_ManageRunLoop();
        // if ( err_code != MQTT_OK )
        // {
        //     diag_printf("ManageRunLoop failed: err_code=%d\n", err_code);
        // }

        // Yield to other tasks...
        osDelay(LAN_TASK_PERIODICITY_MS);
    }
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void)
{
    //////////////////////////////////////////////////////////////////////////
    // Initialize interrupt pin

    // Make sure GPIO is initialized
    nrf_drv_gpiote_init();

    // configure interrupt on falling edge
    nrf_drv_gpiote_in_config_t int_pin_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    if ( nrf_drv_gpiote_in_init( W5500_INTERRUPT_PIN,
                                 &int_pin_config,
                                 int_pin_handler ) != NRF_SUCCESS )
    {
        diag_printf("FAILED TO INITIALIZE W5500 INTERRUPT PIN\n");
    }
    nrf_drv_gpiote_in_event_enable(W5500_INTERRUPT_PIN, true);

    // Initializes the W5500
    LAN_Init();

    // configure interrupts on MQTT socket
    setSIMR(1 << MQTT_SOCK_NUM);

    if ( getSIMR() != (1 << MQTT_SOCK_NUM) )
    {
        diag_printf("FAILED TO ENABLE SOCKET INTERRUPT ON W5500\n");
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Initialize MQTT
    MqttRetCode_t err_code = MqttClient_Init();
    if ( err_code != MQTT_OK )
    {
        diag_printf("FAILED TO CONNECT TO BROKER, err_code=%d\n", err_code);
    }
    //////////////////////////////////////////////////////////////////////////

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
