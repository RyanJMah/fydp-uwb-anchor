#pragma once

#include <stdint.h>
#include "macros.h"
#include "cmsis_os.h"
#include "anchor_config.h"
#include "fira_helper.h"
#include "mqtt_client.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define HEARTBEAT_TOPIC         "gl/anchor/" STR(ANCHOR_ID) "/heartbeat"
#define DATA_TOPIC              "gl/anchor/" STR(ANCHOR_ID) "/data"
#define CONN_REQ_TOPIC          "gl/anchor/" STR(ANCHOR_ID) "/conn/req"
#define CONN_RESP_TOPIC         "gl/anchor/" STR(ANCHOR_ID) "/conn/resp"
#define NI_CONFIG_TOPIC         "gl/anchor/" STR(ANCHOR_ID) "/conn/ni_config"
#define BASE_SELF_CONFIG_TOPIC  "gl/anchor/" STR(ANCHOR_ID) "/config"

/*************************************************************
* TYPE DEFINITIONS
************************************************************/
typedef struct
{
    uint8_t status;
    uint32_t timestamp;
    int32_t distance_mm;
} TelemetryData_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
ALWAYS_INLINE MqttRetCode_t TelemetryData_Publish(struct ranging_measurements* rm)
{
    TelemetryData_t telem_data =
    {
        .status             = rm->status,
        .timestamp          = osKernelSysTick(),
        .distance_mm        = rm->distance_mm,
    };

    return MqttClient_Publish( DATA_TOPIC,
                               &telem_data,
                               sizeof(TelemetryData_t) );
}
