#include "gl_log.h"
#include "flash_memory_map.h"

#include "nrf_delay.h"

int main(void)
{
    GL_LOG_INIT();


    while (1)
    {
        NRF_LOG_INFO("Hello world!");
        NRF_LOG_PROCESS();

        nrf_delay_ms(500);
    }

    return 0;
}

