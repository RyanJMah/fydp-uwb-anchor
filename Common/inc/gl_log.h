#pragma once

#ifdef GL_BOOTLOADER

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "macros.h"

ALWAYS_INLINE void GL_LOG_INIT(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    if (err_code != NRF_SUCCESS)
    {
        /* Can't log an error since logging initialization failed,
         * best we can do is spin in a loop...
         */
        while (1)
        {
            // Do nothing...
        }
    }
}

// #define GL_LOG(fmt, ...)    ( NRF_LOG_INFO(fmt, ##__VA_ARGS__); NRF_LOG_PROCESS() )
#define GL_LOG(fmt, ...)                        \
    do                                          \
    {                                           \
        NRF_LOG_RAW_INFO(fmt, ##__VA_ARGS__);   \
        NRF_LOG_RAW_INFO("\n");                 \
        NRF_LOG_PROCESS();                      \
    }                                           \
    while (0)



#else
// Application

#include "deca_dbg.h"

// Eventually, send logs over MQTT as well
#define GL_LOG(fmt, ...)    diag_printf(fmt, ##__VA_ARGS__)

#endif
