#include "cmsis_os.h"
#include "macros.h"
#include "deca_dbg.h"
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
 * PRIVATE FUNCTIONS
 ************************************************************/
static void _LANTask_Main(void const* args UNUSED)
{
    MqttRetCode_t err_code;

    while (1)
    {
        err_code = MqttClient_ManageRunLoop();
        if ( err_code != MQTT_OK )
        {
            diag_printf("ManageRunLoop failed: err_code=%d\n", err_code);
        }

        // Yield to other tasks...
        osDelay(LAN_TASK_PERIODICITY_MS);
    }
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void)
{
    LAN_Init();

    MqttRetCode_t err_code = MqttClient_Init();
    if ( err_code != MQTT_OK )
    {
        diag_printf("FAILED TO CONNECT TO BROKER, err_code=%d\n", err_code);
    }

    osThreadDef(LANTask, _LANTask_Main, LAN_TASK_PRIORITY, 0, LAN_TASK_STACK_SIZE);
    g_lan_task_id = osThreadCreate( osThread(LANTask), NULL );

    if ( g_lan_task_id == NULL )
    {
        diag_printf("FAILED TO CREATE LAN TASK...\n");
    }
}
