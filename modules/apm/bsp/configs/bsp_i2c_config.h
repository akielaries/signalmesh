#pragma once

#include "hal.h"

// Define GPIO pins for I2C2 on PF0/PF1
#define BSP_I2C2_SCL_PORT GPIOF
#define BSP_I2C2_SCL_PIN  1
#define BSP_I2C2_SDA_PORT GPIOF
#define BSP_I2C2_SDA_PIN  0
#define BSP_I2C2_AF       4 // Alternate Function for I2C2

// Extern declarations for the I2CConfig and I2CDriver instance
extern const I2CConfig bsp_i2c_config;
extern I2CDriver *const bsp_i2c_driver;

void bsp_i2c_init(void);

// Convenience macros for GPIO pins
#define bsp_i2c_scl_port BSP_I2C2_SCL_PORT
#define bsp_i2c_scl_pin  BSP_I2C2_SCL_PIN
#define bsp_i2c_sda_port BSP_I2C2_SDA_PORT
#define bsp_i2c_sda_pin  BSP_I2C2_SDA_PIN
#define bsp_i2c_af       BSP_I2C2_AF

