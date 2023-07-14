#include <stdbool.h>
#include "cmsis_os.h"
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
#define MUTEX_WAIT_TIMEOUT_MS   ( 500 )

/*************************************************************
 * PRIVATE VARIABLES
 ************************************************************/
static uint16_t g_internal_port = 50000;

static osMutexDef(g_lan_mutex);     // Declare mutex
static osMutexId (g_lan_mutex_id);  // Mutex ID

/*************************************************************
 * PRIVATE FUNCTIONS
 ************************************************************/
static ALWAYS_INLINE bool _take_mutex(void)
{
    return osMutexWait(g_lan_mutex_id, MUTEX_WAIT_TIMEOUT_MS) == osOK;
}

static ALWAYS_INLINE void _release_mutex(void)
{
    osMutexRelease(g_lan_mutex_id);
}

/*************************************************************
 * PUBLIC FUNCTIONS
 ************************************************************/

void LAN_Init(void)
{
    diag_printf("Initializing ethernet...\n");

    spi1_master_init();
    user_ethernet_init();

    g_lan_mutex_id = osMutexCreate( osMutex(g_lan_mutex) );

    diag_printf("Ethernet initialized!\n");
}

int16_t LAN_Connect(sock_t sock, ipv4_addr_t addr, uint16_t port)
{
    int16_t err_code;
    bool    mutex_taken = _take_mutex();

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
    if ( mutex_taken )
    {
        _release_mutex();
    }
    return err_code;
}

int32_t LAN_Send(sock_t sock, uint8_t* data, uint32_t len)
{
    bool mutex_taken = _take_mutex();

    int32_t ret = send(sock, data, len);

    if ( mutex_taken )
    {
        _release_mutex();
    }
    return ret;
}

int32_t LAN_Recv(sock_t sock, uint8_t* out_data, uint32_t len)
{
    bool mutex_taken = _take_mutex();

    int32_t ret = recv(sock, out_data, len);

    if ( mutex_taken )
    {
        _release_mutex();
    }
    return ret;
}
