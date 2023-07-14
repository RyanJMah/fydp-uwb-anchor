#pragma once

#include <stdint.h>
#include "cmsis_os.h"
#include "fira_helper.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_PERIODICITY_MS     ( 100 )
#define LAN_TASK_PRIORITY           ( osPriorityBelowNormal )

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

// Call to report TWR data over MQTT
void LANTask_PushToTelemetry(struct ranging_measurements* rm);

void LANTask_Init(void);

