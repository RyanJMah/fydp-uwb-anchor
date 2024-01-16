#pragma once

#include <stdint.h>
#include "transport_interface.h"
#include "lan.h"

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
#define MQTT_CLIENT_IDENTIFIER_FMT      "GuidingLite_Anchor_%u"

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/

// Same as MQTTStatus_t but with some more fields
typedef enum
{
    MQTT_OK = 0,                /**< Function completed successfully. */
    MQTT_BAD_PARAM,             /**< At least one parameter was invalid. */
    MQTT_NO_MEM,                /**< A provided buffer was too small. */
    MQTT_SEND_FAILED,           /**< The transport send function failed. */
    MQTT_RECV_FAILED,           /**< The transport receive function failed. */
    MQTT_BAD_RESPONSE,          /**< An invalid packet was received from the server. */    
    MQTT_SERVER_REFUSED,        /**< The server refused a CONNECT or SUBSCRIBE. */           
    MQTT_NO_DATA_AVAILABLE,     /**< No data available from the transport interface. */         
    MQTT_ILLEGAL_STATE,         /**< An illegal state in the state record. */               
    MQTT_STATE_COLLISION,       /**< A collision with an existing state record entry. */      
    MQTT_KEEP_ALIVE_TIMEOUT,    /**< Timeout while waiting for PINGRESP. */                      
    MQTT_NEED_MORE_BYTES,       /**< MQTT_ProcessLoop/MQTT_ReceiveLoop has received          
                                incomplete data; it should be called again (probably after
                                a delay). */
    MQTT_SOCK_INTERNAL_ERR,     /**< Something bad happend with a W5500 socket */
} MqttRetCode_t;

typedef void ( *MqttClient_SubscribeCallback_t )( char* topic, uint32_t topic_len,
                                                  uint8_t* payload, uint32_t payload_len );

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
int16_t TransportInterface_Init( TransportInterface_t* interface,
                                 ipv4_addr_t broker_addr,
                                 uint32_t    broker_port );

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
MqttRetCode_t MqttClient_Init(void);

MqttRetCode_t MqttClient_ManageRunLoop(void);

MqttRetCode_t MqttClient_Publish(char* topic, void* data, uint32_t len);
MqttRetCode_t MqttClient_Subscribe(char* topic, uint32_t topic_len);

void MqttClient_RegisterSubscribeCallback(MqttClient_SubscribeCallback_t cb);
