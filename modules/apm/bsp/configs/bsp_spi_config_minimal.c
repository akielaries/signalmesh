#include "bsp_spi_config.h"
#include "hal.h"

// SPI1 configuration for W25Q64
// SCK: PA5, MISO: PA6, MOSI: PA7
// CS: PA4 (software controlled)

static const SPIConfig spi1_cfg = {
  .circular = FALSE,
  .end_cb   = NULL,
  .ssport   = 0, // CS managed manually
  .sspad    = 0,
  // STM32H7 uses CFG1/CFG2 registers - using raw values
  .cfg1 = 0x40000007, // PCLK/32, 8-bit data
  .cfg2 = 0x00400000  // Master mode, full duplex
};

// Global SPI bus configuration for SPI1
spi_bus_t spi1_bus_config = {
  .spi_driver = 0, // SPID1 (will be fixed after compilation)
  .spi_config = 0, // &spi1_cfg (will be fixed after compilation)
  .cs_port    = 0, // GPIOA (will be fixed after compilation)
  .cs_pin     = 4,
};

void bsp_spi_init(void) {
  // This will be implemented after getting compilation working
  // Configure SPI1 GPIO pins
  // Start SPI1 driver
}