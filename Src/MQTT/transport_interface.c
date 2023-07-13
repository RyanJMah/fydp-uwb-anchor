#include <stdio.h>
#include <string.h>
#include "transport_interface.h"

struct NetworkContext
{
    uint8_t sock;
    uint8_t server_addr[4];
    uint32_t server_port;
};

static int16_t _NetworkContext_Init(NetworkContext_t* NetworkContext,
                                    uint8_t server_addr[4],
                                    uint32_t server_port )
{
    memcpy( &NetworkContext->server_addr, server_addr, 4*sizeof(uint8_t) );
    NetworkContext->server_port = server_port;
}

int32_t transport_recv(NetworkContext_t* NetworkContext,
                       void* Buffer,
                       size_t BufferSize )
{
    return (int32_t)LAN_Send(Buffer, BufferSize);
}

