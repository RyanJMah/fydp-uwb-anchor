#include "gl_log.h"
#include "socket.h"
#include "flash_memory_map.h"

#include "nrf_delay.h"

int main(void)
{
    GL_LOG_INIT();


    while (1)
    {
        GL_LOG("deez nuts\n");

        nrf_delay_ms(500);
    }

    return 0;
}

