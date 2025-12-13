#include "hal.h"
#include "ch.h"
#include <stdbool.h>
#include <stdint.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/bme280.h"

// Static helper functions
static bool bme280_read_reg_helper(uint8_t reg, uint8_t *val);
static bool bme280_write_reg_helper(uint8_t reg, uint8_t val);
static uint32_t bme280_read_pressure_helper(void);
static uint32_t bme280_read_temperature_helper(void);

static int bme280_init_fn(device_t *dev) {
  (void)dev;
  // BME280 initialization logic
  return DRIVER_OK;
}

static int bme280_probe_fn(device_t *dev) {
  (void)dev;
  uint8_t id;

  bsp_printf("Probing at 0x%02X...\n", BME280_I2C_ADDR);

  // verify we can talk
  if (!bme280_read_reg_helper(BME280_REG_ID, &id)) {
    bsp_printf("Failed to read chip ID!\n");
    return DRIVER_ERROR;
  }
  bsp_printf("Chip ID: 0x%02X\n", id);

  // reset
  if (!bme280_write_reg_helper(BME280_REG_RESET, BME280_SOFT_RESET)) {
    return DRIVER_ERROR;
  }
  bsp_printf("reset finished\n");

  // read original config
  uint8_t config;
  if (!bme280_read_reg_helper(BME280_REG_CONFIG, &config)) {
    return DRIVER_ERROR;
  }
  bsp_printf("Original config: 0x%02X\n", config);

  if (!bme280_write_reg_helper(BME280_REG_CONFIG, BME280_CONFIG_10_MS)) {
    bsp_printf("Failed to write config!\n");
    return DRIVER_ERROR;
  }

  // read back to verify
  uint8_t verify_config;
  if (!bme280_read_reg_helper(BME280_REG_CONFIG, &verify_config)) {
    return DRIVER_ERROR;
  }
  bsp_printf("Verified config: 0x%02X\n", verify_config);

  return DRIVER_OK;
}

static int bme280_remove_fn(device_t *dev) {
  (void)dev;
  // Add removal logic
  return DRIVER_OK;
}

static int bme280_ioctl_fn(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;
  (void)cmd;
  (void)arg;
  return DRIVER_NOT_FOUND;
}

static int bme280_poll_fn(device_t *dev) {
  (void)dev;
  // Add polling logic
  return DRIVER_OK;
}


// Define the driver_t instance for BME280
const driver_t bme280_driver __attribute__((used)) = {
  .name   = "bme280",
  .init   = bme280_init_fn,
  .probe  = bme280_probe_fn,
  .remove = bme280_remove_fn,
  .ioctl  = bme280_ioctl_fn,
  .poll   = bme280_poll_fn,
};

static bool bme280_read_reg_helper(uint8_t reg, uint8_t *val) {
  msg_t msg = i2cMasterTransmitTimeout(&I2CD4,
                                       BME280_I2C_ADDR,
                                       &reg,
                                       1,
                                       val,
                                       1,
                                       TIME_MS2I(1000));

  if (msg != MSG_OK) {
    bsp_printf("[BME280] read_reg error: 0x%X\n", msg);
    return false;
  }

  return true;
}

static bool bme280_write_reg_helper(uint8_t reg, uint8_t val) {
  uint8_t tx[2] = {reg, val};

  msg_t msg = i2cMasterTransmitTimeout(&I2CD4,
                                       BME280_I2C_ADDR,
                                       tx,
                                       2,
                                       NULL,
                                       0,
                                       TIME_MS2I(1000));

  if (msg != MSG_OK) {
    bsp_printf("[BME280] write_reg error: 0x%X\n", msg);
    return false;
  }

  return true;
}

static uint32_t bme280_read_pressure_helper(void) {
  uint8_t data[3];

  if (!bme280_read_reg_helper(BME280_REG_PRESS_ADDR, &data[0]) ||
      !bme280_read_reg_helper(BME280_REG_PRESS_ADDR + 1, &data[1]) ||
      !bme280_read_reg_helper(BME280_REG_PRESS_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

static uint32_t bme280_read_temperature_helper(void) {
  uint8_t data[3];

  if (!bme280_read_reg_helper(BME280_REG_TEMP_ADDR, &data[0]) ||
      !bme280_read_reg_helper(BME280_REG_TEMP_ADDR + 1, &data[1]) ||
      !bme280_read_reg_helper(BME280_REG_TEMP_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}
