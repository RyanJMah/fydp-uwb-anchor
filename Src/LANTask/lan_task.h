#pragma once

#include <stdint.h>
#include "cmsis_os.h"

/*************************************************************
 * MACROS
 ************************************************************/
#define LAN_TASK_PERIODICITY_MS     ( 1000 )
#define LAN_TASK_PRIORITY           ( osPriorityBelowNormal )

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LANTask_Init(void);

