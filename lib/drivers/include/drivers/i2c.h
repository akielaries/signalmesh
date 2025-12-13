#pragma once
#include <stdint.h>
#include <stddef.h>
#include "hal.h"

// Forward declare I2CDriver if it's not defined by hal.h
// This is to avoid circular dependencies if hal.h includes i2c.h indirectly
// If I2CDriver is already defined, this will be ignored.
// It seems I2CDriver is already defined in hal.h based on previous errors.

typedef struct {
  I2CDriver *i2c;
  uint8_t addr;
  systime_t timeout;
} i2c_bus_t;

// New functions for generic I2C communication
msg_t i2c_master_transmit(I2CDriver *i2cp,
                          i2caddr_t addr,
                          const uint8_t *txbuf,
                          size_t txbytes,
                          uint8_t *rxbuf,
                          size_t rxbytes);

msg_t i2c_master_receive(I2CDriver *i2cp,
                         i2caddr_t addr,
                         uint8_t *rxbuf,
                         size_t rxbytes);

void i2c_handle_error(I2CDriver *i2c_driver, const char *context);

// Original functions, adapted to use the new generic functions where
// appropriate
int i2c_bus_read_reg(i2c_bus_t *bus, uint8_t reg, uint8_t *buf, size_t len);
int i2c_bus_write_reg(i2c_bus_t *bus,
                      uint8_t reg,
                      const uint8_t *buf,
                      size_t len);
