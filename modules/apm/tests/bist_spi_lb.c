#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/utils/bsp_io.h"
#include "bsp/bsp.h"
#include "bsp/include/bsp_defs.h"

#define SPI_LOOPBACK_BUFFER_SIZE 32

CC_ALIGN_DATA(32) static uint8_t tx_buf[SPI_LOOPBACK_BUFFER_SIZE];
CC_ALIGN_DATA(32) static uint8_t rx_buf[SPI_LOOPBACK_BUFFER_SIZE];

static const SPIConfig spi1_cfg = {
  .circular = false,
  .data_cb  = NULL,
  .error_cb = NULL,
  .ssport   = GPIOA,
  .sspad    = 4U,
  .cfg1     = SPI_CFG1_MBR_DIV256 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2     = 0U,
};

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

static THD_WORKING_AREA(waSpiLoopThread, 512);
static THD_FUNCTION(SpiLoopThread, arg) {
  (void)arg;
  chRegSetThreadName("SPI1 Loopback");

  uint8_t counter = 0;

  spiStart(&SPID1, &spi1_cfg);

  while (true) {

    for (size_t i = 0; i < SPI_LOOPBACK_BUFFER_SIZE; i++) {
      tx_buf[i] = counter + i;
    }

    bsp_printf("\r\n--- SPI1 loopback %u ---\r\n", counter);
    print_hexdump("TX", tx_buf, SPI_LOOPBACK_BUFFER_SIZE);

    cacheBufferFlush(tx_buf, sizeof tx_buf);
    cacheBufferInvalidate(rx_buf, sizeof rx_buf);

    spiAcquireBus(&SPID1);
    spiSelect(&SPID1);

    spiExchange(&SPID1,
                SPI_LOOPBACK_BUFFER_SIZE,
                tx_buf,
                rx_buf);

    spiUnselect(&SPID1);

    cacheBufferInvalidate(rx_buf, sizeof rx_buf);
    spiReleaseBus(&SPID1);

    print_hexdump("RX", rx_buf, SPI_LOOPBACK_BUFFER_SIZE);

    if (memcmp(tx_buf, rx_buf, SPI_LOOPBACK_BUFFER_SIZE) == 0) {
      bsp_printf("VERIFICATION SUCCESS\r\n");
      palToggleLine(APM_BSP_OK);
    } else {
      bsp_printf("VERIFICATION FAILURE\r\n");
      palToggleLine(APM_BSP_ERR);
    }

    counter++;
    chThdSleepMilliseconds(1000);
  }
}

int main(void) {
  bsp_init();

  bsp_printf("\r\n--- SPI1 MOSI <-> MISO Loopback ---\r\n");

  osalSysLock();

  /* SPI1 pins */
  // SCK
  palSetPadMode(GPIOA, 5,  PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OTYPE_PUSHPULL |
                           PAL_STM32_PUPDR_FLOATING |
                           PAL_STM32_OSPEED_HIGHEST); /* SCK */
  // MISO
  palSetPadMode(GPIOA, 6,  PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OTYPE_PUSHPULL |
                           PAL_STM32_PUPDR_PULLDOWN |
                           PAL_STM32_OSPEED_HIGHEST); /* MISO */
  // MOSI
  palSetPadMode(GPIOA, 7,  PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OTYPE_PUSHPULL |
                           PAL_STM32_PUPDR_FLOATING |
                           PAL_STM32_OSPEED_HIGHEST); /* MOSI */
  // CS
  palSetPadMode(GPIOA, 4, PAL_STM32_MODE_OUTPUT |
                          PAL_STM32_OTYPE_PUSHPULL |
                          PAL_STM32_PUPDR_PULLUP |
                          PAL_STM32_OSPEED_HIGHEST);
  palSetPad(GPIOA, 4);

  osalSysUnlock();
  chThdSleepMicroseconds(100);

  chThdCreateStatic(waSpiLoopThread,
                    sizeof waSpiLoopThread,
                    NORMALPRIO + 1,
                    SpiLoopThread,
                    NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
