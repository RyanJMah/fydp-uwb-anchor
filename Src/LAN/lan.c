#include "macros.h"
#include "user_ethernet.h"
#include "user_spi.h"
#include "w5500.h"
#include "socket.h"
#include "deca_dbg.h"
#include "lan.h"

/*************************************************************
 * MACROS
 ************************************************************/
// #define SOCK_NUM    ( 1 )
// #define TCP_PORT    ( 6900 )

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

// static uint8_t g_server_ip[4] = {169, 254, 0, 1};

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

void LAN_Init(void)
{
    diag_printf("Initializing ethernet...\n");

    spi1_master_init();
    user_ethernet_init();
}

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port)
{
    int16_t err_code;

    diag_printf("creating socket...\n");

    err_code = socket( sock, Sn_MR_TCP, g_internal_port++, 0 );
    require( err_code == SOCK_OK, exit );

    diag_printf(
            "connecting to server hosted at %d.%d.%d.%d\n",
            addr.bytes[0],
            addr.bytes[1],
            addr.bytes[2],
            addr.bytes[3] );

    err_code = connect(sock, addr.bytes, port);
    require( err_code == SOCK_OK, exit );

exit:
    return err_code;
}

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len)
{
    return send(sock, data, len);
}

int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len)
{
    return recv(sock, out_data, len);
}

// not needed yet...
#if 0
int16_t LAN_Update(void)
{
    int16_t err_code;

    switch ( getSn_SR(SOCK_NUM) )
    {
        case SOCK_CLOSED:
        {
            close(SOCK_NUM);

            diag_printf("creating socket...\n");

            err_code = socket(SOCK_NUM, Sn_MR_TCP, internal_port++, 0);
            require( err_code == SOCK_OK, exit );

            break;
        }

        case SOCK_INIT:
        {
            diag_printf("connecting to server hosted at %d.%d.%d.%d\n", g_server_ip[0], g_server_ip[1], g_server_ip[2], g_server_ip[3]);

            err_code = connect(SOCK_NUM, g_server_ip, TCP_PORT);
            require( err_code == SOCK_OK, exit );

            break;
        }

        case SOCK_ESTABLISHED:
        {
            diag_printf("sending data...\n");

            err_code = send(SOCK_NUM, (uint8_t* )send_msg, sizeof(send_msg));
            require( err_code >= 0, exit );

            break;
        }

        default:
        {
            diag_printf("you fucked up bro\n");
            break;
        }
    }

exit:
}
#endif

