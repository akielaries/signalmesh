/**
 * @file bme280.c
 * @brief BME280 environmental sensor driver implementation
 *
 * This file implements the BME280 driver using the standard device
 * driver interface. It provides temperature, pressure, and humidity
 * sensing capabilities over I2C.
 *
 * The driver supports both legacy direct function calls and the new
 * device registry framework for maximum compatibility.
 */

#include "hal.h"
#include "ch.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/bme280.h"
#include "drivers/i2c.h"
#include "drivers/driver_readings.h"

// Macro to calculate the size of an array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Static helper functions */
static bool bme280_read_reg(uint8_t reg, uint8_t *val);
static bool bme280_write_reg(uint8_t reg, uint8_t val);
static uint32_t bme280_read_pressure_raw(void);
static uint32_t bme280_read_temperature_raw(void);

/* Function Prototypes */
static int bme280_init(device_t *dev);
static int bme280_remove(device_t *dev);
static int bme280_ioctl(device_t *dev, uint32_t cmd, void *arg);
static int bme280_poll(device_id_t device_id,
                       uint32_t num_readings,
                       driver_reading_t *readings);

// BME280 specific reading channels
static const driver_reading_channel_t bme280_reading_channels[] = {
  {.name = "temperature", .type = READING_VALUE_TYPE_UINT32},
  {.name = "pressure", .type = READING_VALUE_TYPE_UINT32},
  // Add humidity here when implemented
};

// BME280 readings directory
static const driver_readings_directory_t bme280_readings_directory = {
  .num_readings = ARRAY_SIZE(bme280_reading_channels),
  .channels     = bme280_reading_channels};

/*****************************************************************************/

// Local driver helper functions

/*****************************************************************************/

static bool bme280_read_reg(uint8_t reg, uint8_t *val) {
  uint8_t txbuf = reg;
  msg_t msg = i2c_master_transmit(&I2CD4, BME280_I2C_ADDR, &txbuf, 1, val, 1);

  if (msg != MSG_OK) {
    return false;
  }

  return true;
}

static bool bme280_write_reg(uint8_t reg, uint8_t val) {
  uint8_t tx_data[2] = {reg, val};
  msg_t msg = i2c_master_transmit(&I2CD4, BME280_I2C_ADDR, tx_data, 2, NULL, 0);

  if (msg != MSG_OK) {
    return false;
  }

  return true;
}

static uint32_t bme280_read_pressure_raw(void) {
  uint8_t data[3];

  if (!bme280_read_reg(BME280_REG_PRESS_ADDR, &data[0]) ||
      !bme280_read_reg(BME280_REG_PRESS_ADDR + 1, &data[1]) ||
      !bme280_read_reg(BME280_REG_PRESS_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

static uint32_t bme280_read_temperature_raw(void) {
  uint8_t data[3];

  if (!bme280_read_reg(BME280_REG_TEMP_ADDR, &data[0]) ||
      !bme280_read_reg(BME280_REG_TEMP_ADDR + 1, &data[1]) ||
      !bme280_read_reg(BME280_REG_TEMP_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

/*****************************************************************************/

// Exported driver functions

/*****************************************************************************/

/**
 * @brief Initialize BME280 device
 * @param dev Pointer to device structure
 * @return DRIVER_OK on success, error code on failure
 */
static int bme280_init(device_t *dev) {
  if (dev == NULL) {
    return DRIVER_INVALID_PARAM;
  }

  // Perform soft reset
  if (!bme280_write_reg(BME280_REG_RESET, BME280_SOFT_RESET)) {
    return DRIVER_ERROR;
  }

  chThdSleepMilliseconds(3); // Wait for reset to complete

  // Verify chip ID
  uint8_t chip_id;
  if (!bme280_read_reg(BME280_REG_ID, &chip_id)) {
    return DRIVER_ERROR;
  }

  if (chip_id != BME280_CHIP_ID) {
    return DRIVER_ERROR;
  }

  // Configure sensor (example: standby time 10ms, filter off)
  if (!bme280_write_reg(BME280_REG_CONFIG, BME280_CONFIG_10_MS << 5)) {
    return DRIVER_ERROR;
  }

  return DRIVER_OK;
}

/**
 * @brief Remove BME280 device
 * @param dev Pointer to device structure
 * @return DRIVER_OK on success, error code on failure
 */
static int bme280_remove(device_t *dev) {
  (void)dev;
  // Nothing to de-initialize for now
  return DRIVER_OK;
}

/**
 * @brief Perform device-specific I/O control
 * @param dev Pointer to device structure
 * @param cmd Command identifier
 * @param arg Command-specific argument
 * @return DRIVER_OK on success, error code on failure
 */
static int bme280_ioctl(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;
  (void)cmd;
  (void)arg;
  return DRIVER_NOT_FOUND; // Or DRIVER_NOT_SUPPORTED
}

/**
 * @brief Poll device for new data
 * @param device_id Device identifier
 * @param num_readings Number of readings requested
 * @param readings Array to store reading data
 * @return DRIVER_OK on success, error code on failure
 */
static int bme280_poll(device_id_t device_id,
                       uint32_t num_readings,
                       driver_reading_t *readings) {
  (void)
    device_id; // Unused for now, but could be used for device-specific context

  if (readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > bme280_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM; // Caller asked for more readings than
                                 // available
  }

  // Read raw values
  uint32_t temp_raw  = bme280_read_temperature_raw();
  uint32_t press_raw = bme280_read_pressure_raw();

  // Populate readings array based on directory
  for (uint32_t i = 0; i < num_readings; ++i) {
    if (strcmp(bme280_readings_directory.channels[i].name, "temperature") ==
        0) {
      readings[i].type          = READING_VALUE_TYPE_UINT32;
      readings[i].value.u32_val = temp_raw;
    } else if (strcmp(bme280_readings_directory.channels[i].name, "pressure") ==
               0) {
      readings[i].type          = READING_VALUE_TYPE_UINT32;
      readings[i].value.u32_val = press_raw;
    } else {
      // Handle unknown channel or error
      return DRIVER_ERROR;
    }
  }

  return DRIVER_OK;
}

// Define driver_t instance for BME280
const driver_t bme280_driver __attribute__((used)) = {
  .name               = "bme280",
  .num_data_bytes     = 0, // No specific context struct for now
  .init               = bme280_init,
  .remove             = bme280_remove,
  .ioctl              = bme280_ioctl,
  .poll               = bme280_poll,
  .readings_directory = &bme280_readings_directory,
};