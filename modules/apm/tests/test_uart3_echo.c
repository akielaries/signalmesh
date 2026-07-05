// proves host->MCU on USART3 (PB10 TX / PB11 RX). type in minicom on the dongle:
// each byte is echoed back to minicom and also logged (hex + char) to the UART5
// debug console, so a working RX shows up on both ends. this is the substrate
// the framed update protocol will sit on.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

static const SerialConfig uart3_cfg = {
  .speed = 115200,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_uart3_echo ---\r\n");

  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(7)); // USART3_TX
  palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7)); // USART3_RX
  sdStart(&SD3, &uart3_cfg);

  bsp_printf("USART3 echo ready @ 115200 - type in minicom\r\n");

  for (;;) {
    // blocks until a byte arrives from the dongle
    msg_t m = sdGet(&SD3);
    if (m < 0) {
      continue;
    }
    uint8_t c = (uint8_t)m;
    sdPut(&SD3, c); // echo straight back to minicom
    bsp_printf("rx 0x%02X '%c'\r\n", c, (c >= 0x20 && c < 0x7F) ? c : '.');
  }
}
