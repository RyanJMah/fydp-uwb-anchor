#include "cmsis_os.h"
#include "anchor_config.h"
#include "fira_helper.h"
#include "macros.h"
#include "deca_dbg.h"
#include "custom_board.h"
#include "w5500.h"
#include "nrf_delay.h"
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

    diag_printf("GOT W5500 INTERRUPT: sir=%d\n", getSIR());

    uint8_t sir = getSIR();

    if ( sir & (1 << MQTT_SOCK_NUM) )
    {
        // MQTT socket interrupt, set OS flag to handle in mainloop
        osSignalSet(g_lan_task_id, LAN_TASK_RECV_INTERRUPT_SIGNAL);
    }

    for (uint32_t i = 0; i < 8; i++)
    {
        // If the interrupt bit is set, clear the interrupt register
        if ( sir & (1 << i) )
        {
            setSn_IR(i, 0b1111);
        }
    }
}

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/

// static char* asdf = "this shit is fucked...";
// static uint8_t recv_buff[1024];
// static ipv4_addr_t addr =
// {
//     .bytes = MQTT_BROKER_ADDR
// };

// static void _LANTask_Main(void const* args UNUSED)
// {
//     LAN_Connect(MQTT_SOCK_NUM, addr, 6901);

//     while (1)
//     {
//         // LAN_Send(MQTT_SOCK_NUM, (uint8_t* )asdf, sizeof(asdf));
//         // osDelay(500);
//         LAN_Recv(MQTT_SOCK_NUM, recv_buff, strlen(asdf));
//         diag_printf("%s\n", recv_buff);
//     }
// }

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

    while (1)
    {
        // osSignalWait(LAN_TASK_RECV_INTERRUPT_SIGNAL, osWaitForever);

        // err_code = MqttClient_ManageRunLoop();

        // if ( err_code != MQTT_OK )
        // {
        //     diag_printf("ManageRunLoop failed: err_code=%d\n", err_code);
        // }

        // Yield to other tasks...
        osDelay(LAN_TASK_PERIODICITY_MS);
    }
    while (1)
    {
        osDelay(LAN_TASK_PERIODICITY_MS);
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
