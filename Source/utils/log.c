#include <stdarg.h>
#include <stdio.h>

#include "OSAL.h"
#if defined POWER_SAVING
#include "OSAL_PwrMgr.h"
#endif

#include "hal_uart.h"

#include "uart.h"
#include "log.h"

#define DEBUG_UART      0


#define LOG_TRIES_MAX   3

/* Prototype defined in 8051/src/lib/clib/icclbutl.h */
extern int _formatted_write(const char *, void (*)(char, void *), void *, va_list);

static void logPutOneChar(char c, void *dummy)
{
  uint8 try = LOG_TRIES_MAX;

  while (!HalUARTWrite(DEBUG_UART_PORT, (unsigned char *)&c, 1) && try > 0) {
    try--;
  }

  (void)dummy;  /* Warning on this line OK (Optimized Away) */
}

/* Our main entry */
int logPrintf(const char *format, ...)
{
  va_list ap;
  int nr_of_chars;

#if DEBUG_UART
  P1_1 = !P1_1;
#endif
#if (defined POWER_SAVING && UART_POWER_SAVING)
  /*
   * Workaround for UART working with POWER_SAVING enabled
   */
  // FIXME
  // It may cause system never enter sleep
  if (sUartAlawysOn) {
    UART_RX2GPI();
  }
#endif

  va_start(ap, format);      /* Variable argument begin */
  nr_of_chars = _formatted_write(format, logPutOneChar, (void *)0, ap);
  va_end(ap);                /* Variable argument end */

  return nr_of_chars;        /* According to ANSI */
}

/* vim: set ts=2 sw=2 tw=0 list : */
