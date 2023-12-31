#pragma once

#include <stdint.h>
#include "macros.h"
#include "cmsis_os.h"
#include "fira_helper.h"
#include "mqtt_client.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define HEARTBEAT_TOPIC_FMT         "gl/anchor/%u/heartbeat"
#define DATA_TOPIC_FMT              "gl/anchor/%u/data"
#define CONN_REQ_TOPIC_FMT          "gl/anchor/%u/conn/req"
#define CONN_RESP_TOPIC_FMT         "gl/anchor/%u/conn/resp"
#define NI_CONFIG_TOPIC_FMT         "gl/anchor/%u/conn/ni_config"
#define BASE_SELF_CONFIG_TOPIC_FMT  "gl/anchor/%u/config"
#define DFU_TOPIC_FMT               "gl/anchor/%u/dfu"

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
extern char    g_HEARTBEAT_TOPIC[];
extern uint8_t g_HEARTBEAT_TOPIC_LEN;

extern char    g_DATA_TOPIC[];
extern uint8_t g_DATA_TOPIC_LEN;

extern char    g_CONN_REQ_TOPIC[];
extern uint8_t g_CONN_REQ_TOPIC_LEN;

extern char    g_CONN_RESP_TOPIC[];
extern uint8_t g_CONN_RESP_TOPIC_LEN;

extern char    g_NI_CONFIG_TOPIC[];
extern uint8_t g_NI_CONFIG_TOPIC_LEN;

extern char    g_BASE_SELF_CONFIG_TOPIC[];
extern uint8_t g_BASE_SELF_CONFIG_TOPIC_LEN;

extern char    g_DFU_TOPIC[];
extern uint8_t g_DFU_TOPIC_LEN;

/*************************************************************
* TYPE DEFINITIONS
************************************************************/

/*
 * For status codes, see `enum pctt_status_ranging` in `pctt_region_params.h`
 */

typedef struct
{
    uint8_t status;
    uint32_t timestamp;
    int32_t distance_mm;
} TelemetryData_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void AppMqtt_Init(void);

ALWAYS_INLINE MqttRetCode_t TelemetryData_Publish(struct ranging_measurements* rm)
{
    TelemetryData_t telem_data =
    {
        .status             = rm->status,
        .timestamp          = osKernelSysTick(),
        .distance_mm        = rm->distance_mm,
    };

    return MqttClient_Publish( g_DATA_TOPIC,
                               &telem_data,
                               sizeof(TelemetryData_t) );
}
