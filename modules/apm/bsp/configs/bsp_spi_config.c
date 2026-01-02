#include "bsp/utils/bsp_io.h"
#include "bsp_spi_config.h"
#include "hal.h"

// SPI1 configuration for W25Q64

// SPI Configuration for Master Mode, 8-bit data, CPOL=0, CPHA=0
// baud Rate: PCLK/8 (approx 25MHz if PCLK is 200MHz)
static const SPIConfig spi1_hw_config = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = GPIOA,
  .sspad            = 4U,
  .cfg1             = SPI_CFG1_MBR_DIV64 | SPI_CFG1_DSIZE_VALUE(7),
  .cfg2             = 0U
};

// global SPI bus configuration for SPI1
spi_bus_t spi1_bus_config = {
  .spi_driver = &SPID1,
  .spi_config = (SPIConfig *)&spi1_hw_config,
};


void bsp_spi_init(void) {
  /*
   * SPI1 I/O pins setup.
   */
  // clock
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5));
  // MISO
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_FLOATING);
  // MOSI
  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(5));
  // CS
  palSetPadMode(GPIOA, 4, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_PULLUP);
  //palSetPad(GPIOA, 4);

  // start SPI1 driver
  spiStart(&SPID1, &spi1_hw_config);

  bsp_printf("SPI driver started\n");
}
