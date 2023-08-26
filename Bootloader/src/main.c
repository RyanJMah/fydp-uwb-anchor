#include "gl_log.h"
#include "lan.h"
#include "flash_config_data.h"

#include "anchor_config.h"

#include "nrf_delay.h"

static void w5500_isr( nrf_drv_gpiote_pin_t pin,
                       nrf_gpiote_polarity_t action )
{
    // Empty for now...
}

static ipv4_addr_t server_addr = {.bytes = MQTT_BROKER_ADDR};

static uint8_t msg[] = "Hello World!";

int main(void)
{
    GL_LOG_INIT();

    GL_LOG("Reading config data from flash...\n");
    FlashConfigData_Init();

    GL_LOG("Initializing lan...\n");
    LAN_Init( w5500_isr );

    GL_LOG("Connecting to test server...\n");

    int err_code = LAN_Connect(DHCP_SOCK_NUM, server_addr, 6900);

    if ( err_code <= 0 )
    {
        GL_LOG("Failed to connect to server, err_code = %d\n", err_code);
        while (1)
        {
            // Do nothing...
        }
    }

    while (1)
    {
        GL_LOG("sending msg...\n");
        LAN_Send(DHCP_SOCK_NUM, msg, 12);

        nrf_delay_ms(500);
    }

    return 0;
}

