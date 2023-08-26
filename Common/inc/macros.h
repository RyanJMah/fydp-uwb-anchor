#pragma once

#define UNUSED          __attribute__((unused))
#define ALWAYS_INLINE   inline __attribute__((always_inline))

#define STR_IMPL_(x) #x      // stringify argument
#define STR(x) STR_IMPL_(x)  // indirection to expand argument macros


#include "gl_log.h"

// Assert macros from: https://github.com/apple/darwin-xnu/blob/main/EXTERNAL_HEADERS/AssertMacros.h
#define require_noerr(err_code, exception_label)                            \
    do                                                                      \
    {                                                                       \
        if ( __builtin_expect(0 != (err_code), 0) )                         \
        {                                                                   \
            GL_LOG("Error %d at %s:%d\n", err_code, __FILE__, __LINE__);    \
            goto exception_label;                                           \
        }                                                                   \
    } while ( 0 )

#define require_noerr_action(err_code, exception_label, action)     \
    do                                                              \
    {                                                               \
        if ( __builtin_expect(0 != (err_code), 0) )                 \
        {                                                           \
            {                                                       \
                action;                                             \
            }                                                       \
            goto exception_label;                                   \
        }                                                           \
    } while ( 0 )

#define require(assertion, exception_label)                             \
    do                                                                  \
    {                                                                   \
        if ( __builtin_expect(!(assertion), 0) )                        \
        {                                                               \
            GL_LOG("Error at %s:%d\n", err_code, __FILE__, __LINE__);   \
            goto exception_label;                                       \
        }                                                               \
    } while ( 0 )

#define require_action(assertion, exceptionLabel, action)                   \
    do                                                                      \
    {                                                                       \
        if ( __builtin_expect(!(assertion), 0) )                            \
        {                                                                   \
            {                                                               \
                action;                                                     \
            }                                                               \
            goto exceptionLabel;                                            \
        }                                                                   \
    } while ( 0 )

