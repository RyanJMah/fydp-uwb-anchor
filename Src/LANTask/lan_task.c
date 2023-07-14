#include "cmsis_os.h"
#include "macros.h"
#include "deca_dbg.h"

#include "anchor_config.h"

#include "lan.h"
#include "mqtt_client.h"

#include "lan_task_ringbuff.h"
#include "lan_task.h"
#include <wchar.h>

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_STACK_SIZE     ( 1024U*2 )

// must be a power of two
#define TELEM_DATA_BUFFER_SIZE  ( 1 << 7 )

#define MUTEX_WAIT_TIMEOUT_MS   ( 100 )

#define HEARTBEAT_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/heartbeat"
#define DATA_TOPIC              "/gl/anchor/" STR(ANCHOR_ID) "/data"
#define CONN_REQ_TOPIC          "/gl/anchor/" STR(ANCHOR_ID) "/conn/req"
#define CONN_RESP_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/conn/resp"
#define NI_CONFIG_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/conn/ni_config"
#define BASE_SELF_CONFIG_TOPIC  "/gl/anchor/" STR(ANCHOR_ID) "/config"

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static osThreadId      g_lan_task_id;

static TelemetryData_t g_telem_data_buff[ TELEM_DATA_BUFFER_SIZE ];
static RingBuff_t      g_ringbuff;

static osMutexDef(g_ringbuff_mutex);       // Declare mutex
static osMutexId (g_ringbuff_mutex_id);    // Mutex ID

/*************************************************************
 * IRQ Handlers
 ************************************************************/


/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static ALWAYS_INLINE int _Ringbuff_ThreadSafe_Pop(TelemetryData_t* out_data)
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


static char* fuck = "fuck this shit";
static void _LANTask_Main(void const* args UNUSED)
{
    MqttRetCode_t err_code;

    while (1)
    {
        // while ( !RingBuff_IsEmpty(&g_ringbuff) )
        // {
        //     // Empty the telemetry queue...
        //     diag_printf("LSKDJFLKSDJFLKSDJ\n");

        //     TelemetryData_t data;
        //     _Ringbuff_ThreadSafe_Pop( &data );

        //     err_code = MqttClient_Publish( DATA_TOPIC,
        //                                    (void* )&data,
        //                                    sizeof(TelemetryData_t) );

        //     if ( err_code != MQTT_OK )
        //     {
        //         diag_printf("PUBLISH FAILED: err_code=%d", err_code);
        //     }
        // }

        err_code = MqttClient_Publish(DATA_TOPIC, fuck, strlen(fuck));
        if (err_code != MQTT_OK)
        {
            diag_printf("THIS SHIT DOESN'T WORK");
        }

        // MqttClient_ManageRunLoop();

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

    osThreadDef(LANTask, _LANTask_Main, LAN_TASK_PRIORITY, 0, LAN_TASK_STACK_SIZE);
    g_lan_task_id = osThreadCreate( osThread(LANTask), NULL );

    if ( g_lan_task_id == NULL )
    {
        diag_printf("FAILED TO CREATE LAN TASK...\n");
    }
}
