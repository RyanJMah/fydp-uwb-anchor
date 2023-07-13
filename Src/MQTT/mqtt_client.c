#include <string.h>
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
static MQTTContext_t        g_mqttContext;
static TransportInterface_t g_transport;

static MQTTFixedBuffer_t    g_fixedBuffer;
static uint8_t              g_rx_buffer[ MQTT_CLIENT_RX_BUFF_SIZE ];

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static uint32_t getTimeStampMs()
{
    return osKernelSysTick();
}

// Callback function for receiving packets.
void eventCallback(
     MQTTContext_t * pContext,
     MQTTPacketInfo_t * pPacketInfo,
     MQTTDeserializedInfo_t * pDeserializedInfo
);

// Network send.
int32_t networkSend( NetworkContext_t * pContext, const void * pBuffer, size_t bytes );

// Network receive.
int32_t networkRecv( NetworkContext_t * pContext, void * pBuffer, size_t bytes );

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void MqttClient_Init(void)
{
    // Clear context.
    memset( ( void * ) &mqttContext, 0x00, sizeof( MQTTContext_t ) );

    // Set transport interface members.
    transport.pNetworkContext = &someTransportContext;
    transport.send = networkSend;
    transport.recv = networkRecv;

    // Set buffer members.
    fixedBuffer.pBuffer = buffer;
    fixedBuffer.size = 1024;

    status = MQTT_Init( &mqttContext, &transport, getTimeStampMs, eventCallback, &fixedBuffer );

    if( status == MQTTSuccess )
    {
         // Do something with mqttContext. The transport and fixedBuffer structs were
         // copied into the context, so the original structs do not need to stay in scope.
         // However, the memory pointed to by the fixedBuffer.pBuffer must remain in scope.
    }
}
