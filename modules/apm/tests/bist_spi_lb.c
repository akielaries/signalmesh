#include <string.h>
#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/spi.h"

#define SPI_LOOPBACK_BUFFER_SIZE 256

/*
 * Transmit and receive buffers.
 * These are aligned to ensure they are safe for DMA access.
 */
CC_ALIGN_DATA(32) static uint8_t tx_buf[SPI_LOOPBACK_BUFFER_SIZE];
CC_ALIGN_DATA(32) static uint8_t rx_buf[SPI_LOOPBACK_BUFFER_SIZE];

// Helper to convert a byte to its two-character hex representation
static void byte_to_hex(uint8_t byte, char *hex_str) {
  const char hex_chars[] = "0123456789abcdef";
  hex_str[0]             = hex_chars[(byte >> 4) & 0x0F];
  hex_str[1]             = hex_chars[byte & 0x0F];
}

void print_hexdump(const char *prefix, const uint8_t *data, size_t size) {
  // Max line length: "  0000: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF |................|\n"
  //                   7  + (16 * 3) + 3 + 16 + 1 + 1 = 48 + 16 + 7 = 71, round up to 80 for safety
  char line_buf[80];
  char *ptr;

  bsp_printf("%s (%d bytes):\n", prefix, size);
  for (size_t i = 0; i < size; i += 16) {
    ptr = line_buf;
    // Print address
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "  %04X: ", i); 

    // Print hex bytes
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        byte_to_hex(data[i + j], ptr);
        ptr += 2;
        *ptr++ = ' ';
      } else {
        *ptr++ = ' ';
        *ptr++ = ' ';
        *ptr++ = ' ';
      }   
    }   
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), " |");

    // Print ASCII representation
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        uint8_t c = data[i + j]; 
        *ptr++    = (c >= 32 && c <= 126) ? c : '.';
      } else {
        *ptr++ = ' ';
      }   
    }   
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "|\n");
    bsp_printf("%s", line_buf);
  }
}

/*
 * SPI error callback.
 */
static void spi_error_cb(SPIDriver *spip) {
  (void)spip;
  chSysHalt("SPI hardware error");
}

int main(void) {
    // System initializations.
    halInit();
    chSysInit();
    // Initialize IO for bsp_printf
    bsp_io_init(); 

    bsp_printf("\r\n--- Starting Self-Contained SPI Loopback BIST ---\r\n");

    // --- Local SPI Configuration ---

    // 1. GPIO Setup for SPI1 (SCK: PA5, MISO: PA6, MOSI: PA7, CS: PA15)
    palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SCK
    palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // MISO
    palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // MOSI
    palSetPadMode(GPIOA, 15, PAL_MODE_ALTERNATE(5) | PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST); // CS

    bsp_printf("Configured GPIOs for SPI1 (PA5, PA6, PA7, PA15).\r\n");

    // 2. SPI Driver Configuration struct
    static const SPIConfig local_spi_config = {
      .circular         = false,
      .data_cb          = NULL,
      .error_cb         = spi_error_cb, // Register the error callback
      .ssport           = GPIOA,
      .sspad            = 15U,
      .cfg1             = SPI_CFG1_MBR_DIV8 | SPI_CFG1_DSIZE_VALUE(7), // ~25MHz clock
      .cfg2             = 0U // CPOL=0, CPHA=0 (Mode 0)
    };

    // 3. Start the SPI1 Driver with our local config
    spiStart(&SPID1, &local_spi_config);
    bsp_printf("SPI driver started.\r\n");

    // --- Loopback Test Logic ---

    // Prepare TX buffer with a ramp pattern
    for (int i = 0; i < SPI_LOOPBACK_BUFFER_SIZE; i++) {
        tx_buf[i] = (uint8_t)i;
        rx_buf[i] = 0; // Clear receive buffer
    }
    bsp_printf("Prepared TX buffer with a ramp pattern (0x00 to 0xFF).\r\n");
    print_hexdump("TX_BUF (first 32 bytes)", tx_buf, 32);

    bsp_printf("Performing SPI exchange of %d bytes using polled (non-DMA) functions...\r\n", SPI_LOOPBACK_BUFFER_SIZE);

    // --- Polled (non-DMA) SPI calls ---

    // Acquire bus and select the slave.
    spiAcquireBus(&SPID1);
    spiSelect(&SPID1);

    // Perform the exchange byte-by-byte using the polled function.
    for (int i = 0; i < SPI_LOOPBACK_BUFFER_SIZE; i++) {
        rx_buf[i] = spiPolledExchange(&SPID1, tx_buf[i]);
    }

    // Release the bus.
    spiUnselect(&SPID1);
    spiReleaseBus(&SPID1);

    bsp_printf("SPI exchange complete.\r\n");

    // Verify the received data
    bsp_printf("Verifying data...\r\n");
    if (memcmp(tx_buf, rx_buf, SPI_LOOPBACK_BUFFER_SIZE) == 0) {
        bsp_printf("VERIFICATION SUCCESS: Data read back matches data written.\r\n");
    } else {
        bsp_printf("VERIFICATION FAILURE: Mismatches found between TX and RX buffers!\r\n");
        print_hexdump("TX_BUF (first 32 bytes)", tx_buf, 32);
        print_hexdump("RX_BUF (first 32 bytes)", rx_buf, 32);
    }

    bsp_printf("\r\n--- SPI Loopback BIST Finished ---\r\n");

    // Loop forever
    while (true) {
        chThdSleepMilliseconds(1000);
    }

    return 0;
}
