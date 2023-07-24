#pragma once

#include "macros.h"

#define ANCHOR_ID   0

#define MQTT_BROKER_ADDR            {169, 254, 0, 1}
#define MQTT_BROKER_PORT            1883

#define MQTT_CLIENT_IDENTIFIER      "GuidingLite_Anchor_" STR(ANCHOR_ID)

#define PROJECT_NAME_               "GuidingLite - UWB Anchor Apple NI - FreeRTOS"
#define BOARD_NAME_                 "GuidingLite Anchor PCB"
#define OS_NAME_                    "FreeRTOS"
#define APPLICATION_NAME_           "UWB Anchor Apple NI"
