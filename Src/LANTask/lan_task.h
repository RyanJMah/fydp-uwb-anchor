#pragma once

#include <stdint.h>
#include "cmsis_os.h"
#include "anchor_config.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_PRIORITY               ( osPriorityLow )
#define MQTT_HEARTBEAT_PERIODICITY_MS   ( SOCKET_RECV_TIMEOUT )

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void);

