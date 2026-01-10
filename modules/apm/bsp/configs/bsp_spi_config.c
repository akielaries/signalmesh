#include "bsp/utils/bsp_io.h"
#include "bsp_spi_config.h"
#include "hal.h"

// shared low-level SPI hardware configuration for SPI1.
// NOTE: Chip Select is set to NULL because it is managed manually by
//       the SPI MUX
static const SPIConfig spi1_hw_config = {
  .circular         = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = NULL,
  .sspad            = 0U,
  .cfg1             = SPI_CFG1_MBR_DIV64 |
                      SPI_CFG1_DSIZE_VALUE(7) |
                      SPI_CFG1_FTHLV_VALUE(0),
  .cfg2             = 0U
};

// Device-specific SPI bus configuration for the first W25Qxx flash chip.
spi_bus_t spi_bus_w25qxx_1 = {
  .spi_driver = &SPID1,
  .spi_config = (SPIConfig *)&spi1_hw_config,
  .device_id  = 0, // MUX channel for W25Qxx #1
};

// Device-specific SPI bus configuration for the second W25Qxx flash chip.
spi_bus_t spi_bus_w25qxx_2 = {
  .spi_driver = &SPID1,
  .spi_config = (SPIConfig *)&spi1_hw_config,
  .device_id  = 1, // MUX channel for W25Qxx #2
};


void bsp_spi_init(void) {
  /*
   * SPI1 pin setup
   */
  // SCK
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) );
  // MISO
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) |
                          PAL_STM32_PUPDR_FLOATING );
  // MOSI
  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(5) );

  /*
   * SPI1 74HC138 MUX pin setup. These are the address lines for the chip
   * select "decoder"
   */
  palSetPadMode(GPIOD, 0, PAL_MODE_OUTPUT_PUSHPULL ); // A0
  palSetPadMode(GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL ); // A1
  palSetPadMode(GPIOF, 9, PAL_MODE_OUTPUT_PUSHPULL ); // A2

  // start the underlying SPI1 driver
  spiStart(&SPID1, &spi1_hw_config);

  bsp_printf("SPI MUX driver initialized\n");
}
