#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/utils/bsp_io.h"
#include "bsp/bsp.h"
#include "bsp/include/bsp_defs.h"



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


#define SPI_BUFFER_SIZE 512

CC_ALIGN_DATA(32) static uint8_t tx_buf[SPI_BUFFER_SIZE];
CC_ALIGN_DATA(32) static uint8_t rx_buf[SPI_BUFFER_SIZE];


static const SPIConfig hs_spi_cfg = {
    .circular = false,
    .data_cb  = NULL,
    .error_cb = NULL,
    .ssport   = GPIOA,
    .sspad    = 4U,
    .cfg1     = SPI_CFG1_MBR_DIV256 | SPI_CFG1_DSIZE_VALUE(7),
    .cfg2     = 0U
};

static const SPIConfig ls_spi_cfg = {
    .circular = false,
    .data_cb  = NULL,
    .error_cb = NULL,
    .ssport   = GPIOA,
    .sspad    = 4U,
    .cfg1     = SPI_CFG1_MBR_DIV256 | SPI_CFG1_DSIZE_VALUE(7),
    .cfg2     = 0U
};

#if SPI_SUPPORTS_CIRCULAR == TRUE
static const SPIConfig circ_spi_cfg = {
    .circular = true,
    .data_cb  = NULL,
    .error_cb = NULL,
    .ssport   = GPIOA,
    .sspad    = 4U,
    .cfg1     = SPI_CFG1_MBR_DIV256 | SPI_CFG1_DSIZE_VALUE(7),
    .cfg2     = 0U
};

/* Circular SPI callback */
void spi_circular_cb(SPIDriver *spip) {
    size_t n;

    /* Example button stop simulation */
    if (false) { /* Replace with actual button check if available */
        osalSysLockFromISR();
        spiStopTransferI(spip, &n);
        osalSysUnlockFromISR();
        bsp_printf("[SPI Circular] Transfer stopped by button\r\n");
    }

    if (spiIsBufferComplete(spip)) {
        bsp_printf("[SPI Circular] 2nd half complete\r\n");
    } else {
        bsp_printf("[SPI Circular] 1st half complete\r\n");
    }
}
#endif

/* SPI error callback */
void spi_error_cb(SPIDriver *spip) {
    (void)spip;
    bsp_printf("[SPI ERROR] System halt!\r\n");
    chSysHalt("SPI error");
}

/* SPI thread 1 (high-speed contender) */
static THD_WORKING_AREA(spi_thread_1_wa, 256);
static THD_FUNCTION(spi_thread_1, arg) {
    (void)arg;
    chRegSetThreadName("SPI thread 1");

    while (true) {
        spiAcquireBus(&SPID1);
        bsp_printf("[SPI1 Thread] Starting HS transfer\r\n");

        spiStart(&SPID1, &hs_spi_cfg);
        spiSelect(&SPID1);

        spiExchange(&SPID1, 512, tx_buf, rx_buf);
        spiExchange(&SPID1, 127, tx_buf, rx_buf);
        spiExchange(&SPID1, 64, tx_buf, rx_buf);
        spiExchange(&SPID1, 1, tx_buf, rx_buf);

        spiUnselect(&SPID1);
        cacheBufferInvalidate(rx_buf, sizeof rx_buf);
        spiReleaseBus(&SPID1);

        bsp_printf("[SPI1 Thread] HS transfer complete\r\n");
        chThdSleepMilliseconds(500);
    }
}

/* SPI thread 2 (low-speed contender) */
static THD_WORKING_AREA(spi_thread_2_wa, 256);
static THD_FUNCTION(spi_thread_2, arg) {
    (void)arg;
    chRegSetThreadName("SPI thread 2");

    while (true) {
        spiAcquireBus(&SPID1);
        bsp_printf("[SPI2 Thread] Starting LS transfer\r\n");

        spiStart(&SPID1, &ls_spi_cfg);
        spiSelect(&SPID1);

        spiExchange(&SPID1, 512, tx_buf, rx_buf);
        spiExchange(&SPID1, 127, tx_buf, rx_buf);
        spiExchange(&SPID1, 64, tx_buf, rx_buf);
        spiExchange(&SPID1, 1, tx_buf, rx_buf);

        spiUnselect(&SPID1);
        cacheBufferInvalidate(rx_buf, sizeof rx_buf);
        spiReleaseBus(&SPID1);

        bsp_printf("[SPI2 Thread] LS transfer complete\r\n");
        chThdSleepMilliseconds(500);
    }
}

/* LED blinker thread (status simulation) */
static THD_WORKING_AREA(waBlinkerThread, 256);
static THD_FUNCTION(BlinkerThread, arg) {
    (void)arg;
    chRegSetThreadName("blinker");

    while (true) {
        bsp_printf("[Blinker] Toggle status LED\r\n");
        palToggleLine(APM_BSP_OK);
        chThdSleepMilliseconds(500);
    }
}

/* Application entry point */
int main(void) {
    unsigned i;

    bsp_init();
    bsp_printf("\r\n--- SPI Full Demo Start ---\r\n");

    osalSysLock();
  /* SPI1 pins */
  palSetPadMode(GPIOA, 5,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); /* SCK */
  palSetPadMode(GPIOA, 6,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); /* MISO */
  palSetPadMode(GPIOA, 7,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); /* MOSI */
  //palSetPadMode(GPIOA, 4,  PAL_MODE_ALTERNATE(5) | PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOA, 4, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);

  palSetPad(GPIOA, 4);

  osalSysUnlock();
  chThdSleepMilliseconds(100);

    /* Initialize TX pattern */
    for (i = 0; i < sizeof(tx_buf); i++) {
      tx_buf[i] = (uint8_t)i;
    }
    cacheBufferFlush(tx_buf, sizeof tx_buf);

    /* Start blinker thread */
    chThdCreateStatic(waBlinkerThread, sizeof waBlinkerThread, NORMALPRIO, BlinkerThread, NULL);

    /* Example: DMA + polled mixed SPI transfers */
    spiStart(&SPID1, &ls_spi_cfg);
    do {
        bsp_printf("[Main] Starting mixed polled + DMA transfers\r\n");

        spiSelect(&SPID1);
        spiPolledExchange(&SPID1, tx_buf[0x55]);
        spiPolledExchange(&SPID1, tx_buf[0xAA]);
        spiPolledExchange(&SPID1, tx_buf[0x33]);
        spiPolledExchange(&SPID1, tx_buf[0xCC]);
        spiExchange(&SPID1, 4, tx_buf, rx_buf);
        spiExchange(&SPID1, 3, tx_buf + 8, rx_buf);
        spiUnselect(&SPID1);

        bsp_printf("[Main] Polled + DMA transfer complete\r\n");
        print_hexdump("RX", rx_buf, 16);

        chThdSleepMilliseconds(100);
    } while (false); /* Replace false with actual button check if desired */

#if SPI_SUPPORTS_CIRCULAR == TRUE
    /* Circular DMA test */
    spiStart(&SPID1, &circ_spi_cfg);
    spiSelect(&SPID1);
    spiSend(&SPID1, 512, tx_buf);
    spiUnselect(&SPID1);
    cacheBufferInvalidate(rx_buf, sizeof rx_buf);
    bsp_printf("[Main] Circular DMA test started\r\n");
#endif

    /* Start SPI threads (bus contention demo) */
    chThdCreateStatic(spi_thread_1_wa, sizeof spi_thread_1_wa, NORMALPRIO+1, spi_thread_1, NULL);
    chThdCreateStatic(spi_thread_2_wa, sizeof spi_thread_2_wa, NORMALPRIO+1, spi_thread_2, NULL);

    /* Main loop idle */
    while (true) {
        chThdSleepMilliseconds(500);
    }

    return 0;
}
