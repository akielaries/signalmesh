#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "drivers/driver_api.h"

#define BME280_I2C_ADDR 0x77
#define BME280_CHIP_ID  0x60

#define BME280_REG_ID 0xD0

#define BME280_REG_CONFIG   0xF5
#define BME280_CONFIG_10_MS 0b110
#define BME280_CONFIG_20_MS 0b111

#define BME280_REG_RESET  0xE0
#define BME280_SOFT_RESET 0xB6

#define BME280_REG_PRESS_ADDR 0xF7
#define BME280_REG_PRESS_SIZE 3
#define BME280_REG_TEMP_ADDR  0xFA
#define BME280_REG_TEMP_SIZE  3


extern const driver_t bme280_driver;




bool bme280_probe(void);
uint32_t bme280_read_pressure(void);
uint32_t bme280_read_temperature(void);
