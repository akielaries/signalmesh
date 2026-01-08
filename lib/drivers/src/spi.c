#include <stdint.h>

#include "hal.h"
#include "ch.h"


#include "drivers/spi.h"

#include "bsp/utils/bsp_io.h" // For bsp_printf


void spi_bus_init(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  //spiStart(bus->spi_driver, bus->spi_config);
}

void spi_bus_acquire(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  spiAcquireBus(bus->spi_driver);
  spiSelect(bus->spi_driver);
}

void spi_bus_release(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  spiUnselect(bus->spi_driver);
  spiReleaseBus(bus->spi_driver);
}

void spi_bus_send(spi_bus_t *bus, const uint8_t *txbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || txbuf == NULL || n == 0) {
    return;
  }
  cacheBufferFlush(txbuf, n);
  spiSend(bus->spi_driver, n, txbuf);

}

void spi_bus_receive(spi_bus_t *bus, uint8_t *rxbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || rxbuf == NULL || n == 0) {
    return;
  }


  spiReceive(bus->spi_driver, n, rxbuf);

  cacheBufferInvalidate(&rxbuf[0], n);
  //cacheBufferInvalidate(rxbuf, n);
}

void spi_bus_exchange(spi_bus_t *bus, const uint8_t *txbuf, uint8_t *rxbuf, size_t n) {
  if (bus == NULL || bus->spi_driver == NULL || n == 0) {
    return;
  }
  if (txbuf) {
    // flush TX buffer in memory?
    cacheBufferFlush(txbuf, n);
  }

  spiAcquireBus(bus->spi_driver);
  spiSelect(bus->spi_driver);
  // perform the actual exchange
  spiExchange(bus->spi_driver, n, txbuf, rxbuf);
  spiUnselect(bus->spi_driver);
  spiReleaseBus(bus->spi_driver);

  if (rxbuf) {
    // invalidate the RX buffer in memory?
    cacheBufferInvalidate(rxbuf, n);
  }
}
