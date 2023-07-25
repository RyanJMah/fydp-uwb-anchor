#pragma once

#include "macros.h"

#define ANCHOR_ID   2

#define ANCHOR_MAC_ADDR             {0x00, 0x08, 0xdc,0x00, 0xab, 0xcf}

#define ANCHOR_LAN_USING_DHCP       0

#if !ANCHOR_LAN_USING_DHCP
#define ANCHOR_LAN_IP_ADDR          {192, 168, 8, 5}
#endif

#define ANCHOR_LAN_GW_ADDR          {192, 168, 8, 1}
#define ANCHOR_LAN_SUBNET_MASK      {255, 255, 255, 0}

#define MQTT_BROKER_ADDR            {192, 168, 8, 2}
#define MQTT_BROKER_PORT            1883

#define MQTT_CLIENT_IDENTIFIER      "GuidingLite_Anchor_" STR(ANCHOR_ID)

#define PROJECT_NAME_               "GuidingLite - UWB Anchor Apple NI - FreeRTOS"
#define BOARD_NAME_                 "GuidingLite Anchor PCB"
#define OS_NAME_                    "FreeRTOS"
#define APPLICATION_NAME_           "UWB Anchor Apple NI"
