#include "bsp/utils/bsp_io.h"
#include "bsp_spi_config.h"
#include "hal.h"

// SPI1 configuration for W25Q64
// SCK: PA5, MISO: PA6, MOSI: PA7
// CS: PA15 (managed by SPI driver)

// SPI Configuration for Master Mode, 8-bit data, CPOL=0, CPHA=0
// Baud Rate: PCLK/8 (approx 25MHz if PCLK is 200MHz)
// CS is PA15
static const SPIConfig spi1_hw_config = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = GPIOA,
  .sspad            = 15U,
  .cfg1             = SPI_CFG1_MBR_DIV8 | SPI_CFG1_DSIZE_VALUE(7),
  //.cfg1             = SPI_CFG1_MBR_DIV128 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};

// Global SPI bus configuration for SPI1
spi_bus_t spi1_bus_config = {
  .spi_driver = &SPID1,
  .spi_config = (SPIConfig *)&spi1_hw_config,
};

void bsp_spi_init(void) {
  osalSysLock();
  // Configure SPI1 GPIO pins
  palSetPadMode(GPIOB, 3, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI1 SCK
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI1 MISO
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST); // SPI1 MOSI
  // PA15 as NSS (CS) for SPI1 (alternate function 5)
  palSetPadMode(GPIOA, 15, PAL_MODE_ALTERNATE(5) |
                            PAL_MODE_OUTPUT_PUSHPULL |
                            PAL_STM32_OSPEED_HIGHEST);

  //palSetPad(GPIOA, 15);

  osalSysUnlock();
  chThdSleepMilliseconds(100);
  // Start SPI1 driver
  spiStart(&SPID1, &spi1_hw_config);

  bsp_printf("SPI driver started\n");
}
