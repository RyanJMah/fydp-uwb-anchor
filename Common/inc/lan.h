#pragma once

#include "nrfx_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "socket.h"
#include "macros.h"

/*************************************************************
 * MACROS
 ************************************************************/
// W5500 has 8 HW sockets
#define W5500_NUM_SOCKETS   ( 8 )

#define MQTT_SOCK_NUM       ( 1 )
#define MDNS_SOCK_NUM       ( 2 )
#define DHCP_SOCK_NUM       ( 3 )
#define DFU_SOCK_NUM        ( 4 )

#define MAX_HOSTNAME_CHARS  ( 256 )

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
typedef uint8_t sock_t;

typedef struct
{
    uint8_t bytes[4];
} ipv4_addr_t;

typedef struct
{
    char c[MAX_HOSTNAME_CHARS];
} hostname_t;

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
void LAN_Init(nrfx_gpiote_evt_handler_t isr_func);

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port);

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len);
int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len);

static ALWAYS_INLINE uint8_t IPAddr_IsInvalid(ipv4_addr_t addr)
{
    return (addr.bytes[0] == 0xFF) &&
           (addr.bytes[1] == 0xFF) &&
           (addr.bytes[2] == 0xFF) &&
           (addr.bytes[3] == 0xFF);
}

static ALWAYS_INLINE uint8_t Port_IsInvalid(uint32_t port)
{
    return (port == 0xFFFFFFFF);
}

