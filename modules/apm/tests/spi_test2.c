#include "ch.h"
#include "hal.h"
#include <string.h> // Required for chsnprintf

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#define PORTAB_LINE_LED1            LINE_LED1
#define PORTAB_LINE_LED2            LINE_LED2
#define PORTAB_LED_OFF              PAL_LOW
#define PORTAB_LED_ON               PAL_HIGH
#define PORTAB_LINE_BUTTON          LINE_BUTTON
#define PORTAB_BUTTON_PRESSED       PAL_HIGH

#define PORTAB_SPI1                 SPID1


void print_hexdump(const char *prefix, const uint8_t *data, size_t size) {
    char line_buf[80];
    char *ptr;

    bsp_printf("%s (%u bytes):\r\n", prefix, size);
    for (size_t i = 0; i < size; i += 16) {
        ptr = line_buf;
        ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "  %04X: ", i);

        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                chsnprintf(ptr, 4, "%02X ", data[i + j]);
                ptr += 3;
            } else {
                chsnprintf(ptr, 4, "   ");
                ptr += 3;
            }
        }
        ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), " |");

        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                uint8_t c = data[i + j];
                *ptr++    = (c >= 32 && c <= 126) ? c : '.';
            } else {
                *ptr++ = ' ';
            }
        }
        ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "|\r\n");
        bsp_printf("%s", line_buf);
    }
}

void spi_circular_cb(SPIDriver *spip);
void spi_error_cb(SPIDriver *spip);

/*
 * Circular SPI configuration (25MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig c_spicfg = {
  .circular         = true,
  .data_cb          = spi_circular_cb,
  .error_cb         = spi_error_cb,
  .ssport           = GPIOA,
  .sspad            = 4U,
  .cfg1             = SPI_CFG1_MBR_DIV128 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};

/*
 * Maximum speed SPI configuration (25MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig hs_spicfg = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = spi_error_cb,
  .ssport           = GPIOA,
  .sspad            = 4U,
  .cfg1             = SPI_CFG1_MBR_DIV128 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};

/*
 * Low speed SPI configuration (1.5625MHz, CPHA=0, CPOL=0, MSb first).
 */
const SPIConfig ls_spicfg = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = spi_error_cb,
  .ssport           = GPIOA,
  .sspad            = 4U,
  .cfg1             = SPI_CFG1_MBR_DIV256 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};



/*
 * SPI TX and RX buffers.
 */
CC_ALIGN_DATA(32) static uint8_t txbuf[32];
CC_ALIGN_DATA(32) static uint8_t rxbuf[32];

#if SPI_SUPPORTS_CIRCULAR == TRUE
/*
 * SPI callback for circular operations.
 */
void spi_circular_cb(SPIDriver *spip) {
  size_t n;

#if SPI_SUPPORTS_CIRCULAR == TRUE
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      osalSysLockFromISR();
      spiStopTransferI(&PORTAB_SPI1, &n);
      osalSysUnlockFromISR();
    }
#endif

  if (spiIsBufferComplete(spip)) {
    /* 2nd half.*/
#if defined(PORTAB_LINE_LED1)
    palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);
#endif
  }
  else {
    /* 1st half.*/
#if defined(PORTAB_LINE_LED1)
    palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_ON);
#endif
  }
}
#endif

/*
 * SPI error callback, only used by SPI driver v2.
 */
void spi_error_cb(SPIDriver *spip) {

  (void)spip;

  chSysHalt("SPI error");
}

/*
 * SPI bus contender 1.
 */
/*
static THD_WORKING_AREA(spi_thread_1_wa, 256);
static THD_FUNCTION(spi_thread_1, p) {

  (void)p;
  chRegSetThreadName("SPI thread 1");
  while (true) {
    bsp_printf("\r\n--- SPI thread 1 transfer (HS) ---\r\n");
    spiAcquireBus(&PORTAB_SPI1);        // Acquire ownership of the bus.
#if defined(PORTAB_LINE_LED1)
    palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_ON);
#endif
    spiStart(&PORTAB_SPI1, &hs_spicfg); // Setup transfer parameters.
    spiSelect(&PORTAB_SPI1);            // Slave Select assertion.
    print_hexdump("Thread 1 TX (exchange 32)", txbuf, 32); // Print first 32 bytes
    spiExchange(&PORTAB_SPI1, 32,
                txbuf, rxbuf);          // Atomic transfer operations.
    print_hexdump("Thread 1 RX (exchange 32)", rxbuf, 32); // Print first 32 bytes
    spiExchange(&PORTAB_SPI1, 16,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiExchange(&PORTAB_SPI1, 8,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiExchange(&PORTAB_SPI1, 1,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiUnselect(&PORTAB_SPI1);          // Slave Select de-assertion.
    cacheBufferInvalidate(&rxbuf[0],    // Cache invalidation over the
                          sizeof rxbuf);// buffer.
    spiReleaseBus(&PORTAB_SPI1);        // Ownership release.
    chThdSleepMilliseconds(1000); // Add a small delay to reduce output spam
  }
}
*/

/*
 * SPI bus contender 2.
 */
/*
static THD_WORKING_AREA(spi_thread_2_wa, 256);
static THD_FUNCTION(spi_thread_2, p) {

  (void)p;
  chRegSetThreadName("SPI thread 2");
  while (true) {
    bsp_printf("\r\n--- SPI thread 2 transfer (LS) ---\r\n");
    spiAcquireBus(&PORTAB_SPI1);        // Acquire ownership of the bus.
#if defined(PORTAB_LINE_LED1)
    palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);
#endif
    spiStart(&PORTAB_SPI1, &ls_spicfg); // Setup transfer parameters.
    spiSelect(&PORTAB_SPI1);            // Slave Select assertion.
    print_hexdump("Thread 2 TX (exchange 32)", txbuf, 32); // Print first 32 bytes
    spiExchange(&PORTAB_SPI1, 32,
                txbuf, rxbuf);          // Atomic transfer operations.
    print_hexdump("Thread 2 RX (exchange 32)", rxbuf, 32); // Print first 32 bytes
    spiExchange(&PORTAB_SPI1, 16,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiExchange(&PORTAB_SPI1, 8,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiExchange(&PORTAB_SPI1, 1,
                txbuf, rxbuf);          // Atomic transfer operations.
    spiUnselect(&PORTAB_SPI1);          // Slave Select de-assertion.
    cacheBufferInvalidate(&rxbuf[0],    // Cache invalidation over the
                          sizeof rxbuf);// buffer.
    spiReleaseBus(&PORTAB_SPI1);        // Ownership release.
    chThdSleepMilliseconds(2000); // Add a small delay to reduce output spam
  }
}
*/

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

  /*
   * SPI1 I/O pins setup.
   */
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5)
                          );    /* SPI1 SCK.        */
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_FLOATING
                          );    /* SPI1 MISO.       */
  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(5)
                          );    /* SPI1 MOSI.       */
  palSetPadMode(GPIOA, 4, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_PULLUP
                          );    /* SPI1 NSS.        */
  //palSetPad(GPIOA, 4);

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
  cacheBufferFlush(&txbuf[0], sizeof txbuf);

  bsp_printf("created tx buffer\n");

  /*
   * Start SPI driver.
   */
  spiStart(&PORTAB_SPI1, &ls_spicfg);
  bsp_printf("started SPI driver\n");


  /*
   * Main loop performing a simple, repeatable SPI exchange.
   */
  while (true) {
    bsp_printf("\r\n--- SPI Transaction ---\r\n");
    print_hexdump("TX", txbuf, sizeof(txbuf));

    spiAcquireBus(&PORTAB_SPI1);
    spiSelect(&PORTAB_SPI1);
    spiExchange(&PORTAB_SPI1, sizeof(txbuf), txbuf, rxbuf);
    chThdSleepMicroseconds(10);
    spiUnselect(&PORTAB_SPI1);
    spiReleaseBus(&PORTAB_SPI1);

    cacheBufferInvalidate(&rxbuf[0], sizeof(rxbuf));
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
