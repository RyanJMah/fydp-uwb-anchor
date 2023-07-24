#pragma once

#include <stdint.h>
#include "cmsis_os.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_PRIORITY               ( osPriorityLow )
#define RECV_TIMEOUT                    ( MQTT_RECV_POLLING_TIMEOUT_MS )
#define MQTT_HEARTBEAT_PERIODICITY_MS   ( 5000 )

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void);

