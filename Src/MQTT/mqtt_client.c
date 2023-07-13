#include <string.h>
#include "macros.h"
#include "cmsis_os.h"
#include "core_mqtt.h"
#include "mqtt_client.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define MQTT_CLIENT_RX_BUFF_SIZE    ( 1024*2 )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static MQTTContext_t        g_mqtt_ctx;
static TransportInterface_t g_transport;

static MQTTFixedBuffer_t    g_fixed_buffer;
static uint8_t              g_buffer[ MQTT_CLIENT_RX_BUFF_SIZE ];

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static uint32_t getTimeStampMs()
{
    return osKernelSysTick();
}

// Callback function for receiving packets.
static void eventCallback(
                    MQTTContext_t * pContext,
                    MQTTPacketInfo_t * pPacketInfo,
                    MQTTDeserializedInfo_t * pDeserializedInfo )
{

}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
MqttRetCode_t MqttClient_Init(ipv4_addr_t broker_addr, uint32_t broker_port)
{
    MqttRetCode_t err_code;
    int16_t       sock_err_code;

    // Clear context.
    memset( &g_mqtt_ctx, 0x00, sizeof( MQTTContext_t ) );

    // Initialize transport interface
    sock_err_code = TransportInterface_Init(&g_transport, broker_addr, broker_port);
    require_action( sock_err_code == SOCK_OK, exit, err_code = MQTT_SOCK_INTERNAL_ERR );

    // Set buffer members.
    g_fixed_buffer.pBuffer = g_buffer;
    g_fixed_buffer.size    = sizeof(g_buffer);

    err_code = MQTT_Init(
                    &g_mqtt_ctx,
                    &g_transport,
                    getTimeStampMs,
                    eventCallback,
                    &g_fixed_buffer );
    require_noerr(err_code, exit);

    // Do something with mqttContext. The transport and fixedBuffer structs were
    // copied into the context, so the original structs do not need to stay in scope.
    // However, the memory pointed to by the fixedBuffer.pBuffer must remain in scope.

exit:
    return err_code;
}
