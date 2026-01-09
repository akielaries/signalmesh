#include <stdint.h>

#include "hal.h"
#include "ch.h"

#include "drivers/spi.h"

#include "bsp/utils/bsp_io.h" // for bsp_printf


/**
 * @brief Sets the MUX to select a specific device and selects the SPI bus.
 */
static void spi_bus_cs_select(spi_bus_t *bus) {
  // the device can only be 0 thru 7 for the 74HC138.
  if (bus->device_id > 7) {
    bsp_printf("Device chip select out of range: %d\n", bus->device_id);
    return;
  }

  // set the GPIOs to select the correct MUX channel.
  palWritePad(GPIOD, 0, (bus->device_id >> 0) & 1); // A0
  palWritePad(GPIOD, 1, (bus->device_id >> 1) & 1); // A1
  palWritePad(GPIOF, 9, (bus->device_id >> 2) & 1); // A2

  // select the underlying SPI bus (will do nothing if ssport is NULL, but good practice).
  spiSelect(bus->spi_driver);
}

/**
 * @brief Sets the MUX to a "deselected" state and unselects the SPI bus.
 */
static void spi_bus_cs_unselect(spi_bus_t *bus) {
  // unselect the underlying SPI bus.
  spiUnselect(bus->spi_driver);

  // select a non-existent device (channel 7) to deselect all real ones.
  // this assumes real devices use channels 0 through n-1, where n < 8.
  palWritePad(GPIOD, 0, 1); // A0
  palWritePad(GPIOD, 1, 1); // A1
  palWritePad(GPIOF, 9, 1); // A2
}


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
  spi_bus_cs_select(bus);
}

void spi_bus_release(spi_bus_t *bus) {
  if (bus == NULL || bus->spi_driver == NULL) {
    return;
  }
  spi_bus_cs_unselect(bus);
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
    cacheBufferFlush(txbuf, n);
  }

  spiAcquireBus(bus->spi_driver);
  spi_bus_cs_select(bus);
  // perform the actual exchange
  spiExchange(bus->spi_driver, n, txbuf, rxbuf);
  spi_bus_cs_unselect(bus);
  spiReleaseBus(bus->spi_driver);

  if (rxbuf) {
    // invalidate the RX buffer in memory?
    cacheBufferInvalidate(rxbuf, n);
  }
}
