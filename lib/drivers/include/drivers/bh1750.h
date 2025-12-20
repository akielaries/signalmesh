#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"
#include "drivers/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// BH1750 I2C address
#define BH1750_DEFAULT_ADDRESS 0x23 // ADDR pin is low
#define BH1750_ALT_ADDRESS 0x5C     // ADDR pin is high

// BH1750 Command Opcodes
#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07

// Continuous measurement modes
#define BH1750_CMD_CONT_H_RES_MODE 0x10  // 1 lux resolution, 120ms
#define BH1750_CMD_CONT_H_RES_MODE2 0x11 // 0.5 lux resolution, 120ms
#define BH1750_CMD_CONT_L_RES_MODE 0x13  // 4 lux resolution, 16ms

// One-time measurement modes
#define BH1750_CMD_ONCE_H_RES_MODE 0x20
#define BH1750_CMD_ONCE_H_RES_MODE2 0x21
#define BH1750_CMD_ONCE_L_RES_MODE 0x23

// BH1750 device private data structure
typedef struct {
  i2c_bus_t bus;
} bh1750_t;

extern const driver_t bh1750_driver;

#ifdef __cplusplus
}
#endif
