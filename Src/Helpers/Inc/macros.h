#pragma once

#define UNUSED  __attribute__((unused))

// require_noerr macro taken from: https://github.com/apple/darwin-xnu/blob/main/EXTERNAL_HEADERS/AssertMacros.h
#define require_noerr(err_code, exception_label)        \
    do                                                  \
    {                                                   \
        if ( __builtin_expect(0 != (err_code), 0) )     \
        {                                               \
            goto exception_label;                       \
        }                                               \
    } while ( 0 )

#define require(assertion, exception_label)             \
    do                                                  \
    {                                                   \
        if ( __builtin_expect(!(assertion), 0) )     \
        {                                               \
            goto exception_label;                       \
        }                                               \
    } while ( 0 )

