#pragma once
#include <stdint.h>
#include <stddef.h>
#include "hal.h"


typedef struct {
  I2CDriver *i2c;
  uint8_t addr;
  systime_t timeout;
} i2c_bus_t;


int i2c_bus_read_reg(i2c_bus_t *bus, uint8_t reg, uint8_t *buf, size_t len);
int i2c_bus_write_reg(i2c_bus_t *bus, uint8_t reg, const uint8_t *buf, size_t len);
