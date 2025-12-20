#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"
#include "drivers/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AHT2X_DEFAULT_ADDRESS 0x38

// AHT2x Commands
#define AHT2X_CMD_INITIALIZE 0xBE
#define AHT2X_CMD_TRIGGER_MEASUREMENT 0xAC
#define AHT2X_CMD_SOFT_RESET 0xBA

// AHT2x device private data structure
typedef struct {
  i2c_bus_t bus;
  bool calibrated;
} aht2x_t;

extern const driver_t aht2x_driver;

#ifdef __cplusplus
}
#endif
