#include "ch.h"
#include "hal.h"
#include <string.h>

#include "bsp/bsp.h"
#include "bsp/configs/bsp_spi_config.h"
#include "bsp/utils/bsp_io.h"

#include "common/utils.h"

#include "drivers/spi.h"


#define PORTAB_LINE_LED1            LINE_LED1
#define PORTAB_LINE_LED2            LINE_LED2
#define PORTAB_LED_OFF              PAL_LOW
#define PORTAB_LED_ON               PAL_HIGH
#define PORTAB_LINE_BUTTON          LINE_BUTTON
#define PORTAB_BUTTON_PRESSED       PAL_HIGH

#define PORTAB_SPI1                 SPID1


/*
 * Low speed SPI configuration (1.5625MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig ls_spicfg = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = GPIOA,
  .sspad            = 4U,
  .cfg1             = SPI_CFG1_MBR_DIV64 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};



/*
 * SPI TX and RX buffers.
 */
CC_ALIGN_DATA(32) static uint8_t txbuf[32];
CC_ALIGN_DATA(32) static uint8_t rxbuf[32];

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(blinker_thread_wa, 256);
static THD_FUNCTION(blinker_thread, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    bool key_pressed = palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED;
    systime_t time = key_pressed ? 250 : 500;
#if defined(PORTAB_LINE_LED2)
    palToggleLine(PORTAB_LINE_LED2);
#endif
    chThdSleepMilliseconds(time);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  unsigned i;
  bsp_init();

  bsp_printf("\r\n--- SPI MOSI -> MISO Loopback Test ---\r\n");

  bsp_printf("configured GPIOs\n");

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(blinker_thread_wa, sizeof(blinker_thread_wa), NORMALPRIO, blinker_thread, NULL);

  bsp_printf("created blinker thread...\n");

  /*
   * Prepare transmit pattern.
   */
  for (i = 0; i < sizeof(txbuf); i++) {
    txbuf[i] = (uint8_t)i;
  }
  bsp_printf("created tx buffer\n");

  spi_bus_init(&spi1_bus_config);
  bsp_printf("started SPI driver\n");

  while (true) {
    bsp_printf("\r\n--- SPI Transaction ---\r\n");
    print_hexdump("TX", txbuf, sizeof(txbuf));

    spi_bus_exchange(&spi1_bus_config, txbuf, rxbuf, sizeof(txbuf));

    print_hexdump("RX", rxbuf, sizeof(rxbuf));

    // Compare buffers
    if (memcmp(txbuf, rxbuf, sizeof(txbuf)) == 0) {
      bsp_printf("VERIFICATION SUCCESS\r\n");
    } else {
      bsp_printf("VERIFICATION FAILURE\r\n");
    }

    chThdSleepMilliseconds(1000);
  }
  return 0;
}
