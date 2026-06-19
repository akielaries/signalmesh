// receive the FPGA's 1 Hz "ACM:XX" test messages on UART4 and print each line.
//
// link: FPGA UART_TX (pin 73 / IOT40A) -> STM32 PD0 (UART4_RX). 8N1, 115200,
// common ground. (PD1 = UART4_TX is wired for future bidirectional use.)
//
// the chibios SerialDriver is interrupt-driven: the UART RX ISR fills an input
// queue and sdGetTimeout() suspends this thread until a byte arrives (no busy
// polling). the read loop runs in its own thread so main() stays free.
//
// requires: HAL_USE_SERIAL = TRUE (halconf.h, already on)
//           STM32_SERIAL_USE_UART4 = TRUE (mcuconf.h)

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#define FPGA_UART (&SD4)     // UART4
#define FPGA_BAUD 115200

static const SerialConfig fpga_uart_cfg = {
  .speed = FPGA_BAUD,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

// dedicated receive thread: blocks on the ISR-fed input queue, assembles lines,
// prints each one
static THD_WORKING_AREA(wa_fpga_rx, 1024);
static THD_FUNCTION(fpga_rx_thread, arg) {
  (void)arg;
  chRegSetThreadName("fpga_rx");

  char   line[64];
  size_t len = 0;

  while (true) {
    msg_t c = sdGetTimeout(FPGA_UART, TIME_MS2I(3000));
    if (c < 0) {
      // MSG_TIMEOUT / MSG_RESET: nothing arrived
      bsp_printf("(no data for 3s)\n");
      continue;
    }

    char ch = (char)c;
    if (ch == '\n') {
      if (len > 0 && line[len - 1] == '\r') {
        len--;                       // drop trailing CR
      }
      line[len] = '\0';
      bsp_printf("FPGA: %s\n", line);
      len = 0;
    }
    else if (len < sizeof line - 1) {
      line[len++] = ch;
    }
    else {
      line[len] = '\0';
      bsp_printf("FPGA (truncated): %s\n", line);
      len = 0;
    }
  }
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fpga_uart: listening on UART4 (PD0) ---\r\n");

  // UART4 pins: PD0 = RX (AF8). PD1 = TX (AF8), wired but unused here
  palSetPadMode(GPIOD, 0, PAL_MODE_ALTERNATE(8));   // RX
  palSetPadMode(GPIOD, 1, PAL_MODE_ALTERNATE(8));   // TX (future)

  sdStart(FPGA_UART, &fpga_uart_cfg);
  bsp_printf("UART4 @ %u baud, waiting for FPGA...\n", (unsigned)FPGA_BAUD);

  // spawn the receive thread; main is then free
  chThdCreateStatic(wa_fpga_rx, sizeof wa_fpga_rx, NORMALPRIO,
                    fpga_rx_thread, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
