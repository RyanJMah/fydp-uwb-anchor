#include "nrf_delay.h"

#include "gl_log.h"
#include "lan.h"
#include "dfu.h"
#include "flash_config_data.h"

static void w5500_isr( nrf_drv_gpiote_pin_t pin,
                       nrf_gpiote_polarity_t action )
{
    // Empty for now...
}

static uint8_t msg[] = "Hello World!";

int main(void)
{
    GL_LOG_INIT();

    GL_LOG("ENTERING BOOTLOADER!\n");

    GL_LOG("Reading config data from flash...\n");
    FlashConfigData_Init();

    if ( !gp_persistent_conf->fw_update_pending )
    {
        // Jump straight to application code
        DFU_JumpToApp();
    }

    // If we're here, the fw_update_pending flag is set, start DFU

    LAN_Init( w5500_isr );

    GL_LOG("Connecting to dfu server...\n");

    ret_code_t err_code;
    int        sock_err_code;

    // Find a server to connect to
    for (uint8_t i = 0; i < NUM_FALLACK_SERVERS; i++)
    {
        if ( Port_IsInvalid(gp_persistent_conf->server_port[i]) ||
             IPAddr_IsInvalid(gp_persistent_conf->server_ip_addr[i]) )
        {
            continue;
        }

        sock_err_code = LAN_Connect(DFU_SOCK_NUM, gp_persistent_conf->server_ip_addr[0], 6900);

        if ( sock_err_code > 0 )
        {
            break;
        }

        GL_LOG("Failed to connect to server %d, trying next...\n", i);
    }

    if ( sock_err_code <= 0 )
    {
        GL_LOG("Failed to connect to any server, aborting...\n");
        goto err_handler;
    }

    // Mainloop
    while (1)
    {
        GL_LOG("sending msg...\n");
        LAN_Send(DFU_SOCK_NUM, msg, 12);

        nrf_delay_ms(500);
    }

err_handler:
    GL_LOG("DFU FATAL ERROR: err_code=%d, sock_err_code=%d\n", err_code, sock_err_code);

    // Auto-reboot after 5 seconds
    nrf_delay_ms(5000);
    NVIC_SystemReset();

    return 0;
}

