#pragma once

#include <stdint.h>
#include "cmsis_os.h"
// #include "anchor_config.h"
#include "flash_config_data.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_PRIORITY               ( osPriorityLow )
#define MQTT_HEARTBEAT_PERIODICITY_MS   ( g_persistent_conf.socket_recv_timeout_ms )

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void);

