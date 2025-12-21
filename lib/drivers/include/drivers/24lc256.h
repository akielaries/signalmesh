#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"
#include "drivers/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// 24LC256 I2C address
#define EEPROM_24LC256_DEFAULT_ADDRESS 0x50

// 32768 bytes, 32kb
#define EEPROM_24LC256_SIZE_BYTES      (32 * 1024)
#define EEPROM_24LC256_PAGE_SIZE_BYTES 64

// 24LC256 device private data structure
typedef struct {
  i2c_bus_t bus;
} eeprom_24lc256_t;

extern const driver_t eeprom_24lc256_driver;

#ifdef __cplusplus
}
#endif
