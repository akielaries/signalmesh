#include <stdint.h>
#include <string.h> // For memcpy
#include "hal.h"
#include "drivers/driver_api.h"
#include "drivers/i2c.h"
#include "bsp/utils/bsp_io.h"


// General purpose I2C error handler
void i2c_handle_error(I2CDriver *i2c_driver, const char *context) {
  i2cflags_t err = i2cGetErrors(i2c_driver);
  if (err != I2C_NO_ERROR) {
    bsp_printf("I2C error in %s: ", context);
    if (err & I2C_BUS_ERROR) {
      bsp_printf("BUS ERROR\n");
    } else if (err & I2C_ARBITRATION_LOST) {
      bsp_printf("ARBITRATION LOST\n");
    } else if (err & I2C_ACK_FAILURE) {
      bsp_printf("ACK FAILURE\n");
    } else if (err & I2C_OVERRUN) {
      bsp_printf("OVERRUN\n");
    } else {
      bsp_printf("unknown error code: 0x%02X\n", err);
    }
    // Attempt to recover the I2C bus
    // i2cStop(i2c_driver);
    // i2cStart(i2c_driver, i2c_driver->config); // Requires driver config
    // access bsp_printf("I2C bus recovered (%s)\n", context);
  }
}

// Generic I2C Master Transmit function
msg_t i2c_master_transmit(I2CDriver *i2cp,
                          i2caddr_t addr,
                          const uint8_t *txbuf,
                          size_t txbytes,
                          uint8_t *rxbuf,
                          size_t rxbytes) {
  msg_t msg;
  i2cAcquireBus(i2cp);
  msg = i2cMasterTransmitTimeout(i2cp, addr, txbuf, txbytes, rxbuf, rxbytes, TIME_MS2I(100));
  i2cReleaseBus(i2cp);
  if (msg != MSG_OK) {
    i2c_handle_error(i2cp, "i2c_master_transmit");
  }
  return msg;
}

// Generic I2C Master Receive function
msg_t i2c_master_receive(I2CDriver *i2cp, i2caddr_t addr, uint8_t *rxbuf, size_t rxbytes) {
  msg_t msg;
  i2cAcquireBus(i2cp);
  msg = i2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxbytes, TIME_MS2I(100));
  i2cReleaseBus(i2cp);
  if (msg != MSG_OK) {
    i2c_handle_error(i2cp, "i2c_master_receive");
  }
  return msg;
}


int i2c_bus_read_reg(i2c_bus_t *bus, uint8_t reg, uint8_t *buf, size_t len) {
  uint8_t tx_buf[1];
  tx_buf[0] = reg;
  msg_t msg = i2c_master_transmit(bus->i2c, bus->addr, tx_buf, 1, buf, len);
  return (msg == MSG_OK) ? DRV_OK : DRV_EIO;
}

int i2c_bus_write_reg(i2c_bus_t *bus, uint8_t reg, const uint8_t *buf, size_t len) {
  uint8_t tx_buf[len + 1];
  tx_buf[0] = reg;
  memcpy(&tx_buf[1], buf, len);
  msg_t msg = i2c_master_transmit(bus->i2c, bus->addr, tx_buf, len + 1, NULL, 0);
  return (msg == MSG_OK) ? DRV_OK : DRV_EIO;
}
