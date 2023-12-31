#include <stdio.h>
#include <string.h>
#include "macros.h"
#include "gl_log.h"
#include "lan.h"
#include "transport_interface.h"

/*************************************************************
 * TYPE DEFINITIONS
 ************************************************************/
struct NetworkContext
{
    sock_t sock;
};

/*************************************************************
 * GLOBAL VARIABLES
 ************************************************************/
static NetworkContext_t g_network_ctx;

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static int32_t transport_recv( NetworkContext_t* ctx,
                               void* buff,
                               size_t buff_size )
{
    return LAN_Recv(ctx->sock, buff, buff_size);
}

static int32_t transport_send( NetworkContext_t* ctx,
                               const void* buff,
                               size_t len )
{
    return LAN_Send(ctx->sock, (uint8_t* )buff, len);
}

static int16_t NetworkContext_Init( NetworkContext_t* ctx,
                                    ipv4_addr_t server_addr,
                                    uint32_t server_port )
{
    ctx->sock = MQTT_SOCK_NUM;

    return LAN_Connect(ctx->sock, server_addr, server_port);
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/
int16_t TransportInterface_Init( TransportInterface_t* interface,
                                 ipv4_addr_t broker_addr,
                                 uint32_t    broker_port )
{
    int16_t err_code;

    GL_LOG("Attempting to connect to server...\n");

    err_code = NetworkContext_Init(&g_network_ctx, broker_addr, broker_port);
    require( err_code == SOCK_OK, exit );

    GL_LOG("Successfully connected to server!\n");

    interface->recv            = transport_recv;
    interface->send            = transport_send;
    interface->pNetworkContext = &g_network_ctx;

exit:
    return err_code;
}
