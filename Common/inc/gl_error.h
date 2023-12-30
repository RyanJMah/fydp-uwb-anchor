#pragma once

#include "nrf_delay.h"
#include "macros.h"
#include "gl_log.h"

#define GL_FATAL_ERROR()                                            \
    do                                                              \
    {                                                               \
        GL_LOG("Fatal error at %s:%d\n", __FILENAME__, __LINE__);   \
        nrf_delay_ms(500);                                             \
        NVIC_SystemReset();                                         \
    }                                                               \
    while (0)
