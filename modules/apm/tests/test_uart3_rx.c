// rx data on UART 3 for a POC

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"


// USART3 at 115200 8N1
static const SerialConfig uart3_cfg = {
  .speed = 115200,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};


// chprintf target for the dongle link
static BaseSequentialStream *const uart3 = (BaseSequentialStream *)&SD3;


int main(void) {
  bsp_init();
  bsp_printf("\n--- test_uart3_hello ---\r\n");

  // PB10 = USART3_TX, PB11 = USART3_RX, both AF7 on H7
  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7));
  sdStart(&SD3, &uart3_cfg);

  bsp_printf("USART3 up on PB10/PB11 @ 115200\n");

  uint32_t n = 0U;
  for (;;) {
    chprintf(uart3, "ACM UART3 hello %lu\r\n", (unsigned long)n++);
    bsp_printf("sent frame %lu on USART3\r\n", (unsigned long)n);
    chThdSleepMilliseconds(1000);
  }
}
