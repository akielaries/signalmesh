#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "hal.h"
#include <string.h>

void bsp_io_init(void) {
  // these come from the Port <port #> alternate functions in datasheet/stm32h753vi.pdf
  palSetPadMode(GPIOC,
                12,
                PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD,
                2,
                PAL_MODE_ALTERNATE(8));
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
