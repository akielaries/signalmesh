#pragma once

#include "hal.h"
#include "drivers/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

// External declaration for the SPI bus configuration for SPI1
extern spi_bus_t spi_bus_w25qxx_1;
extern spi_bus_t spi_bus_w25qxx_2;

/**
 * @brief Initializes the SPI peripheral(s) and their associated GPIO pins.
 *
 * This function sets up the SPI master driver(s) and configures the GPIO
 * pins for SCK, MISO, MOSI, and Chip Select (CS) for each SPI bus used.
 * It should be called once during system startup.
 */
void bsp_spi_init(void);

#ifdef __cplusplus
}
#endif
