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
#define HEARTBEAT_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/heartbeat"
#define DATA_TOPIC              "/gl/anchor/" STR(ANCHOR_ID) "/data"
#define CONN_REQ_TOPIC          "/gl/anchor/" STR(ANCHOR_ID) "/conn/req"
#define CONN_RESP_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/conn/resp"
#define NI_CONFIG_TOPIC         "/gl/anchor/" STR(ANCHOR_ID) "/conn/ni_config"
#define BASE_SELF_CONFIG_TOPIC  "/gl/anchor/" STR(ANCHOR_ID) "/config"

/*************************************************************
* TYPE DEFINITIONS
************************************************************/

typedef struct
{
    uint8_t rssi;
    uint8_t status;

    uint32_t timestamp;

    int32_t distance_mm;

    /*
     * The angles are represented as a 16-bit fixed-point number
     * with 0 integer bits and 16 fractional bits (Q0.16).
     *
     * The angles are also represented as "how many multiples
     * of 2pi" the angle is. So for example, 0.5 corresponds to
     * 0.5*2pi = pi = 180 degrees.
     *
     * Thus, the angles in this struct can be converted to
     * degrees like so:
     *
     * float convert_aoa_2pi_q16_to_deg(int16_t aoa_2pi_q16)
     * {
     *     return (360.0 * aoa_2pi_q16 / (1 << 16));
     * }
     */

    int16_t iphone_aoa_2pi_q16;
    uint8_t iphone_aoa_fom;         // fom: figure-of-merit

    int16_t anchor_aoa_2pi_q16;
    uint8_t anchor_aoa_fom;
} TelemetryData_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
extern MqttRetCode_t TelemetryData_Publish(struct ranging_measurements* rm);
ALWAYS_INLINE MqttRetCode_t TelemetryData_Publish(struct ranging_measurements* rm)
{
    TelemetryData_t telem_data =
    {
        .rssi               = rm->rssi,
        .status             = rm->status,
        .timestamp          = osKernelSysTick(),
        .distance_mm        = rm->distance_mm,
        .iphone_aoa_2pi_q16 = rm->remote_aoa_azimuth_2pi,
        .iphone_aoa_fom     = rm->remote_aoa_azimuth_fom,
        .anchor_aoa_2pi_q16 = rm->local_aoa_measurements[0].aoa_2pi,
        .anchor_aoa_fom     = rm->local_aoa_measurements[0].aoa_fom
    };

    return MqttClient_Publish( DATA_TOPIC,
                               &telem_data,
                               sizeof(TelemetryData_t) );
}
