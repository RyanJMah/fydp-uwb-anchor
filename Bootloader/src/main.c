#include "gl_log.h"
#include "lan.h"
#include "flash_memory_map.h"

#include "nrf_delay.h"

static void w5500_isr( nrf_drv_gpiote_pin_t pin,
                       nrf_gpiote_polarity_t action )
{
    // Empty for now...
}

int main(void)
{
    GL_LOG_INIT();

    GL_LOG("Initializing lan...");
    LAN_Init( w5500_isr );

    while (1)
    {
        GL_LOG("asdlfk");

        nrf_delay_ms(500);
    }

    return 0;
}

