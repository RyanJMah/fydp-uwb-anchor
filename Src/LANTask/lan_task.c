#include "cmsis_os.h"
#include "macros.h"
#include "deca_dbg.h"

#include "lan.h"
#include "mqtt_client.h"

#include "lan_task_ringbuff.h"
#include "lan_task.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_STACK_SIZE     ( 1024U*10 )

// must be a power of two
#define TELEM_DATA_BUFFER_SIZE  ( 32U )

#define MUTEX_WAIT_TIMEOUT_MS   ( 1000U )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static TelemetryData_t g_telem_data_buff[ TELEM_DATA_BUFFER_SIZE ];
static RingBuff_t      g_ringbuff;

osMutexDef(g_ringbuff_mutex);       // Declare mutex
osMutexId (g_ringbuff_mutex_id);    // Mutex ID

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static ALWAYS_INLINE int _Ringbuff_Pop_ThreadSafe(TelemetryData_t* out_data)
{
    osStatus err_code = osMutexWait(g_ringbuff_mutex_id, osWaitForever);

    if ( err_code == osOK )
    {
        // If mutex was successfully taken...

        int ret = RingBuff_Pop(&g_ringbuff, out_data);
        osMutexRelease(g_ringbuff_mutex_id);

        return ret;
    }
    else
    {
        return -1;
    }
}

static void _LANTask_Main(void const* args UNUSED)
{
    while (1)
    {
        // while ( !RingBuff_IsEmpty(&g_ringbuff) )
        // {
        //     // Force context switch...
        //     osDelay(1);
        // }

        // If we've gotten here, there is now stuff in the ringbuff...


        MqttClient_ManageRunLoop();

        // Yield to other tasks...
        osDelay(LAN_TASK_PERIODICITY_MS);
    }
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_PushToTelemetry(struct ranging_measurements* rm)
{
    osStatus err_code = osMutexWait(g_ringbuff_mutex_id, MUTEX_WAIT_TIMEOUT_MS);

    if ( err_code == osOK )
    {
        // If mutex was successfully taken...

        RingBuff_Push(&g_ringbuff, rm);
        osMutexRelease(g_ringbuff_mutex_id);
    }
}

void LANTask_Init(void)
{
    g_ringbuff_mutex_id = osMutexCreate( osMutex(g_ringbuff_mutex) );
    RingBuff_Init(&g_ringbuff, g_telem_data_buff, TELEM_DATA_BUFFER_SIZE);

    LAN_Init();

    MqttRetCode_t err_code = MqttClient_Init();
    if ( err_code != MQTT_OK )
    {
        diag_printf("FAILED TO CONNECT TO BROKER, err_code=%d\n", err_code);
    }

    osThreadDef(LANTask, _LANTask_Main, osPriorityNormal, 0, LAN_TASK_STACK_SIZE);
    osThreadId id = osThreadCreate( osThread(LANTask), NULL );

    if ( id == NULL )
    {
        diag_printf("FAILED TO CREATE LAN TASK...\n");
    }
}
