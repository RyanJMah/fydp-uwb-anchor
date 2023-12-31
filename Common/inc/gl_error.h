#pragma once

#include "nrf_delay.h"
#include "macros.h"
#include "gl_log.h"

#define GL_FATAL_ERROR()                                            \
    do                                                              \
    {                                                               \
        GL_LOG("Fatal error at %s:%d\n", __FILENAME__, __LINE__);   \
        nrf_delay_ms(500);                                          \
        NVIC_SystemReset();                                         \
    }                                                               \
    while (0)

#define GL_ERROR_CHECK(__err_code)                                  \
    do                                                              \
    {                                                               \
        if ( (__err_code) != NRF_SUCCESS)                           \
        {                                                           \
            GL_LOG("Error at %s:%d\n", __FILENAME__, __LINE__);     \
            GL_FATAL_ERROR();                                       \
        }                                                           \
    }                                                               \
    while (0)
