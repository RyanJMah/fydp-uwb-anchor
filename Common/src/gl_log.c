#ifdef GL_BOOTLOADER

// Copy and pasted from deca_dbg.c, too lazy to compile it into the bootloader

#include <stdarg.h>
#include <string.h>
#include "SEGGER_RTT.h"

#define DIAG_BUF_LEN 0x8
#define DIAG_STR_LEN 64

typedef struct
{
    uint8_t buf[DIAG_BUF_LEN][DIAG_STR_LEN];
    int     head;
} gDiagPrintFStr_t;

gDiagPrintFStr_t gDiagPrintFStr;

static void rtt_print(char *buff, int len)
{
    SEGGER_RTT_WriteNoLock(0, buff, len);
}

void diag_printf(char *s, ...)
{
    va_list args;
    va_start(args, s);
    vsnprintf((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0]), DIAG_STR_LEN, s, args);
    rtt_print((char *)&gDiagPrintFStr.buf[gDiagPrintFStr.head][0], strlen((char *)(&gDiagPrintFStr.buf[gDiagPrintFStr.head][0])));
    gDiagPrintFStr.head = (gDiagPrintFStr.head + 1) & (DIAG_BUF_LEN - 1);
    va_end(args);
}

#endif
