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
  .cfg1             = SPI_CFG1_MBR_DIV16 |
                      SPI_CFG1_DSIZE_VALUE(7) |
                      SPI_CFG1_FTHLV_VALUE(0),
  .cfg2             = 0U
};

// global SPI bus configuration for SPI1
spi_bus_t spi1_bus_config = {
  .spi_driver = &SPID1,
  .spi_config = (SPIConfig *)&spi1_hw_config,
};


void bsp_spi_init(void) {
  // 74HC134 setup for the chip select MUX
  palSetPadMode(GPIOD, 0, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOF, 9, PAL_MODE_OUTPUT_PUSHPULL);

  // SPI1 pin setup
  // clock
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  // MISO
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_FLOATING | PAL_STM32_OSPEED_HIGHEST);
  // MOSI
  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  // CS
  palSetPadMode(GPIOA, 4, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_PULLUP | PAL_STM32_OSPEED_HIGHEST);
  //palSetPad(GPIOA, 4);

  // start SPI1 driver
  spiStart(&SPID1, &spi1_hw_config);
  //spi_bus_init(&spi1_bus_config);

  bsp_printf("SPI driver started\n");
}
