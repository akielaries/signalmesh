#include <stdint.h>
#include "hal.h"
#include "drivers/driver_api.h"
#include "drivers/i2c.h"


int i2c_bus_read_reg(i2c_bus_t *bus, uint8_t reg, uint8_t *buf, size_t len) {
  msg_t msg = i2cMasterTransmitTimeout(bus->i2c,
                                       bus->addr,
                                       &reg,
                                       1,
                                       buf,
                                       len,
                                       bus->timeout);

  return (msg == MSG_OK) ? DRV_OK : DRV_EIO;
}
