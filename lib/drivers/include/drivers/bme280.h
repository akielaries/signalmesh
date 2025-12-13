/**
 * @file bme280.h
 * @brief BME280 temperature, pressure, and humidity sensor driver
 *
 * This driver provides support for the Bosch BME280 combined environmental
 * sensor. It supports temperature, pressure, and humidity measurements
 * over I2C interface.
 *
 * The driver implements the standard device driver interface and can be
 * used with the device registry system.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver_api.h"

/** @brief Default I2C address for BME280 */
#define BME280_I2C_ADDR 0x77

/** @brief Expected chip ID for BME280 */
#define BME280_CHIP_ID 0x60

/* Register addresses */
/** @brief Chip ID register */
#define BME280_REG_ID 0xD0

/** @brief Configuration register */
#define BME280_REG_CONFIG 0xF5
/** @brief Configuration: 10ms standby time */
#define BME280_CONFIG_10_MS 0b110
/** @brief Configuration: 20ms standby time */
#define BME280_CONFIG_20_MS 0b111

/** @brief Soft reset register */
#define BME280_REG_RESET 0xE0
/** @brief Soft reset command value */
#define BME280_SOFT_RESET 0xB6

/** @brief Pressure measurement registers (3 bytes, 20-bit value) */
#define BME280_REG_PRESS_ADDR 0xF7
#define BME280_REG_PRESS_SIZE 3

/** @brief Temperature measurement registers (3 bytes, 20-bit value) */
#define BME280_REG_TEMP_ADDR 0xFA
#define BME280_REG_TEMP_SIZE 3

/**
 * @brief External driver instance declaration
 *
 * This is the main driver structure that gets registered with the
 * device registry system.
 */
extern const driver_t bme280_driver;

/**
 * @brief Legacy probe function for standalone testing
 *
 * This function is provided for compatibility with existing test code.
 * New code should use the driver framework instead.
 *
 * @return true if probe successful, false otherwise
 */
bool bme280_probe(void);

/**
 * @brief Read raw pressure value
 *
 * @return 20-bit pressure measurement value, 0 on error
 */
uint32_t bme280_read_pressure(void);

/**
 * @brief Read raw temperature value
 *
 * @return 20-bit temperature measurement value, 0 on error
 */
uint32_t bme280_read_temperature(void);
