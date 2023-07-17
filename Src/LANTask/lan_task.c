#include "cmsis_os.h"
#include "nrf_delay.h"
#include "fira_helper.h"
#include "macros.h"
#include "deca_dbg.h"
#include "custom_board.h"
#include "w5500.h"
#include "nrf_drv_gpiote.h"
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
static void interrupt_pin_handler( nrf_drv_gpiote_pin_t pin,
                                   nrf_gpiote_polarity_t action )
{
    if ( pin != W5500_INTERRUPT_PIN )
    {
        return;
    }

    // read socket interrupt register, make sure interrupt came from MQTT
    // if ( (getSIR() & (1 << MQTT_SOCK_NUM)) == 0 )
    // {
    //     return;
    // }

    diag_printf("GOT W5500 INTERRUPT: sir=%d\n", getSIR());
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
    ////////////////////////////////////////////////////////////////////////////
    // Initialize interrupt pin

    // Initializes the W5500
    LAN_Init( interrupt_pin_handler );

    ////////////////////////////////////////////////////////////////////////////

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
