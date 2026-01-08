#pragma once

#include <stdint.h>
#include <stddef.h>
#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI bus configuration structure
 *
 * Encapsulates all information needed to communicate with an SPI device.
 * This provides a clean interface between device drivers and the underlying
 * ChibiOS SPI implementation.
 */
typedef struct {
  /** @brief Pointer to ChibiOS SPI driver instance */
  SPIDriver *spi_driver;

  /** @brief Pointer to SPI configuration structure */
  SPIConfig *spi_config;
} spi_bus_t;



void spi_cs_mux_select(uint8_t device);


void spi_bus_init(spi_bus_t *bus);

/**
 * @brief Acquires the SPI bus and asserts the Chip Select (CS) line.
 *
 * This function must be called before any SPI data transfer operations
 * within a transaction.
 *
 * @param[in] bus Pointer to SPI bus configuration.
 */
void spi_bus_acquire(spi_bus_t *bus);

/**
 * @brief De-asserts the Chip Select (CS) line and releases the SPI bus.
 *
 * This function must be called after all SPI data transfer operations
 * within a transaction are complete.
 *
 * @param[in] bus Pointer to SPI bus configuration.
 */
void spi_bus_release(spi_bus_t *bus);

/**
 * @brief Sends data over the SPI bus.
 *
 * This function assumes the bus has already been acquired and CS asserted
 * by `spi_bus_acquire()`.
 *
 * @param[in] bus Pointer to SPI bus configuration.
 * @param[in] txbuf Pointer to transmit buffer.
 * @param[in] n Number of bytes to transmit.
 */
void spi_bus_send(spi_bus_t *bus, const uint8_t *txbuf, size_t n);

/**
 * @brief Receives data from the SPI bus.
 *
 * This function assumes the bus has already been acquired and CS asserted
 * by `spi_bus_acquire()`.
 *
 * @param[in] bus Pointer to SPI bus configuration.
 * @param[in/out] rxbuf Pointer to receive buffer.
 * @param[in] n Number of bytes to receive.
 */
void spi_bus_receive(spi_bus_t *bus, uint8_t *rxbuf, size_t n);

/**
 * @brief Exchanges data over the SPI bus (send and receive simultaneously).
 *
 * @param[in] bus Pointer to SPI bus configuration.
 * @param[in] txbuf Pointer to transmit buffer (can be NULL if only receiving).
 * @param[in/out] rxbuf Pointer to receive buffer (can be NULL if only transmitting).
 * @param[in] n Number of bytes to exchange.
 */
void spi_bus_exchange(spi_bus_t *bus, const uint8_t *txbuf, uint8_t *rxbuf, size_t n);

#ifdef __cplusplus
}
#endif
