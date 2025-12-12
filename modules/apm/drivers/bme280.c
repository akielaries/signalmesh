#include "hal.h"
#include "ch.h"
#include <stdbool.h>
#include <stdint.h>
#include "bsp/utils/bsp_io.h"
#include "bme280.h"


bool bme280_read_reg(uint8_t reg, uint8_t *val) {
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

bool bme280_write_reg(uint8_t reg, uint8_t val) {
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

uint32_t bme280_read_pressure(void) {
  uint8_t data[3];

  if (!bme280_read_reg(BME280_REG_PRESS_ADDR, &data[0]) ||
      !bme280_read_reg(BME280_REG_PRESS_ADDR + 1, &data[1]) ||
      !bme280_read_reg(BME280_REG_PRESS_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

uint32_t bme280_read_temperature(void) {
  uint8_t data[3];

  if (!bme280_read_reg(BME280_REG_TEMP_ADDR, &data[0]) ||
      !bme280_read_reg(BME280_REG_TEMP_ADDR + 1, &data[1]) ||
      !bme280_read_reg(BME280_REG_TEMP_ADDR + 2, &data[2])) {
    return 0;
  }

  // Combine 20-bit value: MSB<<16 | LSB<<8 | XLSB
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
}

bool bme280_probe(void) {
  uint8_t id;

  bsp_printf("Probing at 0x%02X...\n", BME280_I2C_ADDR);

  // verify we can talk
  if (!bme280_read_reg(BME280_REG_ID, &id)) {
    bsp_printf("Failed to read chip ID!\n");
    return false;
  }
  bsp_printf("Chip ID: 0x%02X\n", id);

  // reset
  if (!bme280_write_reg(BME280_REG_RESET, BME280_SOFT_RESET)) {
    return false;
  }
  bsp_printf("reset finished\n");

  // read original config
  uint8_t config;
  if (!bme280_read_reg(BME280_REG_CONFIG, &config)) {
    return false;
  }
  bsp_printf("Original config: 0x%02X\n", config);

  if (!bme280_write_reg(BME280_REG_CONFIG, BME280_CONFIG_10_MS)) {
    bsp_printf("Failed to write config!\n");
    return false;
  }

  // read back to verify
  uint8_t verify_config;
  if (!bme280_read_reg(BME280_REG_CONFIG, &verify_config)) {
    return false;
  }
  bsp_printf("Verified config: 0x%02X\n", verify_config);

  while (1) {

    uint32_t temp = bme280_read_temperature();
    bsp_printf("raw temp: %d\n", temp);

    uint32_t press = bme280_read_pressure();
    bsp_printf("raw pressure: %d\n", press);

    chThdSleepMilliseconds(500);
  }
  return true;
}
