#include "drivers/spi.h"
#include "hal.h"
#include "bsp/utils/bsp_io.h" // For bsp_printf

/**
 * @brief Acquires the SPI bus and asserts the Chip Select (CS) line.
 *
 * This function must be called before any SPI data transfer operations
 * within a transaction.
 *
 * @param bus Pointer to SPI bus configuration.
 */
void spi_bus_acquire(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  spiAcquireBus(bus->spi_driver);
  spiSelect(bus->spi_driver);
}

/**
 * @brief De-asserts the Chip Select (CS) line and releases the SPI bus.
 *
 * This function must be called after all SPI data transfer operations
 * within a transaction are complete.
 *
 * @param bus Pointer to SPI bus configuration.
 */
void spi_bus_release(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  spiUnselect(bus->spi_driver);
  spiReleaseBus(bus->spi_driver);
}

/**
 * @brief Sends data over the SPI bus.
 *
 * This function assumes the bus has already been acquired and CS asserted
 * by `spi_bus_acquire()`.
 *
 * @param bus Pointer to SPI bus configuration.
 * @param txbuf Pointer to transmit buffer.
 * @param n Number of bytes to transmit.
 */
void spi_bus_send(spi_bus_t *bus, const uint8_t *txbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || txbuf == NULL || n == 0) {
    return;
  }
  cacheBufferFlush(txbuf, n);
  spiSend(bus->spi_driver, n, txbuf);
}

/**
 * @brief Receives data from the SPI bus.
 *
 * This function assumes the bus has already been acquired and CS asserted
 * by `spi_bus_acquire()`.
 *
 * @param bus Pointer to SPI bus configuration.
 * @param rxbuf Pointer to receive buffer.
 * @param n Number of bytes to receive.
 */
void spi_bus_receive(spi_bus_t *bus, uint8_t *rxbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || rxbuf == NULL || n == 0) {
    return;
  }
  spiReceive(bus->spi_driver, n, rxbuf);
  cacheBufferInvalidate(rxbuf, n);
}

/**
 * @brief Exchanges data over the SPI bus (send and receive simultaneously).
 *
 * This function assumes the bus has already been acquired and CS asserted
 * by `spi_bus_acquire()`.
 *
 * @param bus Pointer to SPI bus configuration.
 * @param txbuf Pointer to transmit buffer (can be NULL if only receiving).
 * @param rxbuf Pointer to receive buffer (can be NULL if only transmitting).
 * @param n Number of bytes to exchange.
 */
void spi_bus_exchange(spi_bus_t *bus, const uint8_t *txbuf, uint8_t *rxbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || n == 0) {
    return;
  }
  cacheBufferFlush(txbuf, n);
  spiExchange(bus->spi_driver, n, txbuf, rxbuf);
  cacheBufferInvalidate(rxbuf, n);
}
