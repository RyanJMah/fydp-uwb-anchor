#include <string.h>
#include "deca_dbg.h"
#include "macros.h"
#include "cmsis_os.h"
#include "core_mqtt.h"
#include "anchor_config.h"
#include "mqtt_client.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define MQTT_CLIENT_RX_BUFF_SIZE    ( 1024*2 )

#define OUTGOING_PUBLISH_BUFF_LEN   ( 10 )
#define INCOMING_PUBLISH_BUFF_LEN   ( 10 )

#define KEEP_ALIVE_TIMEOUT_SECONDS  ( 60 )

#define CONNACK_RECV_TIMEOUT_MS     ( 1000 )

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static MQTTContext_t        g_mqtt_ctx;
static TransportInterface_t g_transport;

static MQTTFixedBuffer_t    g_fixed_buffer;
static uint8_t              g_buffer[ MQTT_CLIENT_RX_BUFF_SIZE ];

static MQTTPubAckInfo_t     g_outgoing_publishes[ OUTGOING_PUBLISH_BUFF_LEN ];
static MQTTPubAckInfo_t     g_incoming_publishes[ INCOMING_PUBLISH_BUFF_LEN ];

static MQTTConnectInfo_t    g_connection_info;

const ipv4_addr_t g_broker_addr =
{
    .bytes = MQTT_BROKER_ADDR
};

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static uint32_t getTimeStampMs()
{
    return osKernelSysTick();
}

// Callback function for receiving packets.
static void eventCallback( MQTTContext_t * pContext,
                           MQTTPacketInfo_t * pPacketInfo,
                           MQTTDeserializedInfo_t * pDeserializedInfo )
{

}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
MqttRetCode_t MqttClient_Init(void)
{
    MqttRetCode_t err_code;
    int16_t       sock_err_code;

    // Clear context...
    memset( &g_mqtt_ctx, 0x00, sizeof(g_mqtt_ctx) );

    // Initialize transport interface...
    sock_err_code = TransportInterface_Init( &g_transport,
                                              g_broker_addr,
                                              MQTT_BROKER_PORT );
    require_action( sock_err_code == SOCK_OK, exit, err_code = MQTT_SOCK_INTERNAL_ERR );

    // Set buffer members.
    g_fixed_buffer.pBuffer = g_buffer;
    g_fixed_buffer.size    = sizeof(g_buffer);

    err_code = MQTT_Init( &g_mqtt_ctx,
                          &g_transport,
                          getTimeStampMs,
                          eventCallback,
                          &g_fixed_buffer );
    require_noerr(err_code, exit);

    err_code = MQTT_InitStatefulQoS( &g_mqtt_ctx,
                                     g_outgoing_publishes,
                                     OUTGOING_PUBLISH_BUFF_LEN,
                                     g_incoming_publishes,
                                     INCOMING_PUBLISH_BUFF_LEN );
    require_noerr(err_code, exit);

    // Clear connection info struct...
   memset( &g_connection_info, 0, sizeof(g_connection_info) ); 

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    g_connection_info.cleanSession = true;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    g_connection_info.pClientIdentifier = MQTT_CLIENT_IDENTIFIER;
    g_connection_info.clientIdentifierLength = (uint16_t)strlen(MQTT_CLIENT_IDENTIFIER);

    /* Set MQTT keep-alive period. If the application does not send packets at an interval less than
     * the keep-alive period, the MQTT library will send PINGREQ packets. */
    g_connection_info.keepAliveSeconds = KEEP_ALIVE_TIMEOUT_SECONDS;

    bool session_present UNUSED;
    err_code = MQTT_Connect( &g_mqtt_ctx,
                             &g_connection_info,
                             NULL,
                             CONNACK_RECV_TIMEOUT_MS,
                             &session_present );
    require_noerr(err_code, exit);

    diag_printf("Successfully connected to MQTT broker...\n");

exit:
    return err_code;
}
