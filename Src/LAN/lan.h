#pragma once

#include "socket.h"

/*************************************************************
 * MACROS
 ************************************************************/
// W5500 has 8 HW sockets
#define W5500_NUM_SOCKETS   ( 8 )

#define MQTT_SOCK_NUM       ( 1 )
#define MDNS_SOCK_NUM       ( 2 )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef uint8_t sock_t;

typedef struct
{
    uint8_t bytes[4];
} ipv4_addr_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

void LAN_Init();

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port);

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len);
int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len);


