#include "bsp/configs/bsp_uart_config.h"

// Configuration for the debug serial port (UART5)
const SerialConfig bsp_debug_uart_config = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

// Pointer to the SerialDriver instance to use (UART5)
SerialDriver * const bsp_debug_uart_driver = &SD5;
// Pointer to the BaseSequentialStream for chprintf
BaseSequentialStream *bsp_debug_stream = (BaseSequentialStream *)&SD5;
