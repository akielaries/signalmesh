#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "hal.h"
#include <string.h>

void bsp_io_init(void) {
  // Configure GPIOs for the debug UART
  palSetPadMode(BSP_DEBUG_UART_TX_PORT,
                BSP_DEBUG_UART_TX_PIN,
                PAL_MODE_ALTERNATE(BSP_DEBUG_UART_AF));
  palSetPadMode(BSP_DEBUG_UART_RX_PORT,
                BSP_DEBUG_UART_RX_PIN,
                PAL_MODE_ALTERNATE(BSP_DEBUG_UART_AF));

  // Start the serial driver
  sdStart(bsp_debug_uart_driver, &bsp_debug_uart_config);
}

char bsp_io_getchar(void) {
  char c;
  chnRead(bsp_debug_stream, (uint8_t *)&c, 1);
  return c;
}

void bsp_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  chvprintf(bsp_debug_stream, fmt, ap);
  chprintf(bsp_debug_stream, "\r");
  va_end(ap);
}
