/**
 * @file bme280.c
 * @brief BME280 environmental sensor driver implementation
 *
 * This file implements the BME280 driver using the standard device
 * driver interface. It provides temperature, pressure, and humidity
 * sensing capabilities over I2C.
 */
#include "ch.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/bme280.h"
#include "drivers/driver_readings.h"
#include "drivers/i2c.h"

// Macro to calculate the size of an array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/******************************************************************************/
// Function prototypes
/******************************************************************************/

static int bme280_init(device_t *dev);
static int bme280_remove(device_t *dev);
static int bme280_ioctl(device_t *dev, uint32_t cmd, void *arg);
static int bme280_poll(device_id_t device_id,
                       uint32_t num_readings,
                       driver_reading_t *readings);


/******************************************************************************/
// Local types
/******************************************************************************/

// BME280 specific reading channels
static const driver_reading_channel_t bme280_reading_channels[] = {
  {
    .name = "temperature",
    .unit = "C",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "pressure",
    .unit = "Pa",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "humidity",
    .unit = "%RH",
    .type = READING_VALUE_TYPE_FLOAT,
  },
};

// BME280 readings directory
static const driver_readings_directory_t bme280_readings_directory = {
  .num_readings = ARRAY_SIZE(bme280_reading_channels),
  .channels     = bme280_reading_channels
};

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

/******************************************************************************/
// Local driver helper functions
/******************************************************************************/

static int bme280_read_registers(bme280_t *bme_dev,
                                 uint8_t reg,
                                 uint8_t *buf,
                                 size_t len) {
  return i2c_bus_read_reg(&bme_dev->bus, reg, buf, len);
}

static int bme280_write_register(bme280_t *bme_dev,
                                 uint8_t reg,
                                 const uint8_t *buf,
                                 size_t len) {
  return i2c_bus_write_reg(&bme_dev->bus, reg, buf, len);
}

static uint32_t bme280_read_pressure_raw(bme280_t *bme_dev) {
  uint8_t data[3];
  bme280_read_registers(bme_dev, BME280_REG_PRESS_MSB, data, 3);
  return ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) |
         ((uint32_t)data[2] >> 4);
}

static uint32_t bme280_read_temperature_raw(bme280_t *bme_dev) {
  uint8_t data[3];
  bme280_read_registers(bme_dev, BME280_REG_TEMP_MSB, data, 3);
  return ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) |
         ((uint32_t)data[2] >> 4);
}

static uint32_t bme280_read_humidity_raw(bme280_t *bme_dev) {
  uint8_t data[2];
  bme280_read_registers(bme_dev, BME280_REG_HUM_MSB, data, 2);
  return ((uint32_t)data[0] << 8) | (uint32_t)data[1];
}

static bool bme280_read_calib_params(bme280_t *bme_dev) {
  uint8_t data[26];
  bme280_read_registers(bme_dev, 0x88, data, 26);

  bme_dev->calib_params.dig_T1 = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
  bme_dev->calib_params.dig_T2 = (int16_t)(((int16_t)data[3] << 8) | data[2]);
  bme_dev->calib_params.dig_T3 = (int16_t)(((int16_t)data[5] << 8) | data[4]);

  bme_dev->calib_params.dig_P1 = (uint16_t)(((uint16_t)data[7] << 8) | data[6]);
  bme_dev->calib_params.dig_P2 = (int16_t)(((int16_t)data[9] << 8) | data[8]);
  bme_dev->calib_params.dig_P3 =
    (int16_t)(((int16_t)data[11] << 8) | data[10]);
  bme_dev->calib_params.dig_P4 =
    (int16_t)(((int16_t)data[13] << 8) | data[12]);
  bme_dev->calib_params.dig_P5 =
    (int16_t)(((int16_t)data[15] << 8) | data[14]);
  bme_dev->calib_params.dig_P6 =
    (int16_t)(((int16_t)data[17] << 8) | data[16]);
  bme_dev->calib_params.dig_P7 =
    (int16_t)(((int16_t)data[19] << 8) | data[18]);
  bme_dev->calib_params.dig_P8 =
    (int16_t)(((int16_t)data[21] << 8) | data[20]);
  bme_dev->calib_params.dig_P9 =
    (int16_t)(((int16_t)data[23] << 8) | data[22]);

  bme280_read_registers(bme_dev, 0xA1, &bme_dev->calib_params.dig_H1, 1);

  uint8_t hum_data[7];
  bme280_read_registers(bme_dev, 0xE1, hum_data, 7);

  bme_dev->calib_params.dig_H2 =
    (int16_t)(((int16_t)hum_data[1] << 8) | hum_data[0]);
  bme_dev->calib_params.dig_H3 = hum_data[2];
  bme_dev->calib_params.dig_H4 =
    (int16_t)(((int16_t)hum_data[3] << 4) | (hum_data[4] & 0x0F));
  bme_dev->calib_params.dig_H5 =
    (int16_t)(((int16_t)hum_data[5] << 4) | ((hum_data[4] >> 4) & 0x0F));
  bme_dev->calib_params.dig_H6 = (int8_t)hum_data[6];

  return true;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of 5123
// equals 51.23 DegC. t_fine carries fine temperature as global value
static int32_t bme280_compensate_temperature_int32(int32_t adc_T,
                                                   bme280_t *bme_dev) {
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)bme_dev->calib_params.dig_T1 << 1))) *
          ((int32_t)bme_dev->calib_params.dig_T2)) >>
         11;
  var2 = (((((adc_T >> 4) - ((int32_t)bme_dev->calib_params.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)bme_dev->calib_params.dig_T1))) >>
           12) *
          ((int32_t)bme_dev->calib_params.dig_T3)) >>
         14;
  bme_dev->t_fine = var1 + var2;
  T               = (bme_dev->t_fine * 5 + 128) >> 8;
  return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer
// bits and 8 fractional bits). Output value of 24674867 equals 24674867/256 =
// 96386.2 Pa = 963.862 hPa
static uint32_t bme280_compensate_pressure_int64(int32_t adc_P,
                                                 bme280_t *bme_dev) {
  int64_t var1, var2, p;
  var1 = ((int64_t)bme_dev->t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)bme_dev->calib_params.dig_P6;
  var2 = var2 + ((var1 * (int64_t)bme_dev->calib_params.dig_P5) << 17);
  var2 = var2 + (((int64_t)bme_dev->calib_params.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)bme_dev->calib_params.dig_P3) >> 8) +
         ((var1 * (int64_t)bme_dev->calib_params.dig_P2) << 12);
  var1 =
    (((((int64_t)1) << 47) + var1)) * ((int64_t)bme_dev->calib_params.dig_P1) >>
    33;
  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 =
    (((int64_t)bme_dev->calib_params.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)bme_dev->calib_params.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)bme_dev->calib_params.dig_P7) << 4);
  return (uint32_t)p;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22
// integer and 10 fractional bits). Output value of 47445 equals 47445/1024
// = 46.333 %RH
static uint32_t bme280_compensate_humidity_int32(int32_t adc_H,
                                                 bme280_t *bme_dev) {
  int32_t v_x1_u32r;
  v_x1_u32r = (bme_dev->t_fine - ((int32_t)76800));
  v_x1_u32r =
    (((((adc_H << 14) - (((int32_t)bme_dev->calib_params.dig_H4) << 20) -
        (((int32_t)bme_dev->calib_params.dig_H5) * v_x1_u32r)) +
       ((int32_t)16384)) >>
      15) *
     (((((((v_x1_u32r * ((int32_t)bme_dev->calib_params.dig_H6)) >> 10) *
          (((v_x1_u32r * ((int32_t)bme_dev->calib_params.dig_H3)) >> 11) +
           ((int32_t)32768))) >>
         10) +
        ((int32_t)2097152)) *
         ((int32_t)bme_dev->calib_params.dig_H2) +
       8192) >>
      14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                             ((int32_t)bme_dev->calib_params.dig_H1)) >>
                            4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (uint32_t)(v_x1_u32r >> 12);
}

/******************************************************************************/
// Exported driver functions
/******************************************************************************/

/**
 * @brief Initialize BME280 device
 * @param dev Pointer to device structure
 * @return DRIVER_OK on success, error code on failure
 */
static int bme280_init(device_t *dev) {
  if (dev == NULL) {
    return DRIVER_INVALID_PARAM;
  }

  bme280_t *bme_dev = (bme280_t *)chHeapAlloc(NULL, sizeof(bme280_t));
  if (bme_dev == NULL) {
    return DRIVER_ERROR;
  }
  memset(bme_dev, 0, sizeof(bme280_t));
  dev->priv = bme_dev;

  // Initialize I2C bus
  bme_dev->bus.i2c = (I2CDriver *)dev->bus;
  bme_dev->bus.addr = BME280_I2C_ADDR;

  // Read calibration parameters
  if (!bme280_read_calib_params(bme_dev)) {
    chHeapFree(bme_dev);
    return DRIVER_ERROR;
  }

  // Perform soft reset
  uint8_t reset_cmd = BME280_SOFT_RESET;
  if (bme280_write_register(bme_dev, BME280_REG_RESET, &reset_cmd, 1) != 0) {
    return DRIVER_ERROR;
  }

  chThdSleepMilliseconds(3); // Wait for reset to complete

  // Verify chip ID
  uint8_t chip_id;
  if (bme280_read_registers(bme_dev, BME280_REG_ID, &chip_id, 1) != 0) {
    return DRIVER_ERROR;
  }

  if (chip_id != BME280_CHIP_ID) {
    return DRIVER_ERROR;
  }

  // Configure sensor (example: standby time 10ms, filter off)
  uint8_t config = BME280_CONFIG_T_SB_10MS << 5;
  if (bme280_write_register(bme_dev, BME280_REG_CONFIG, &config, 1) != 0) {
    return DRIVER_ERROR;
  }

  // Configure humidity oversampling
  uint8_t ctrl_hum = BME280_OVERSAMPLING_X16 << BME280_CTRL_HUM_OSRS_H_POS;
  if (bme280_write_register(bme_dev, BME280_REG_CTRL_HUM, &ctrl_hum, 1) != 0) {
    return DRIVER_ERROR;
  }

  // Configure pressure oversampling, temperature oversampling and set to normal
  // mode
  uint8_t ctrl_meas_val =
    (BME280_OVERSAMPLING_X16 << BME280_CTRL_MEAS_OSRS_T_POS) |
    (BME280_OVERSAMPLING_X16 << BME280_CTRL_MEAS_OSRS_P_POS) |
    (BME280_MODE_NORMAL << BME280_CTRL_MEAS_MODE_POS);
  if (bme280_write_register(bme_dev, BME280_REG_CTRL_MEAS, &ctrl_meas_val, 1) != 0) {
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
  bme280_t *bme_dev = (bme280_t *)device_id->priv;

  if (bme_dev == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > bme280_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM; // Caller asked for more readings than
                                 // available
  }

  // Read raw values
  int32_t temp_raw  = bme280_read_temperature_raw(bme_dev);
  int32_t press_raw = bme280_read_pressure_raw(bme_dev);
  int32_t hum_raw   = bme280_read_humidity_raw(bme_dev);

  // Compensate values
  int32_t compensated_temp =
    bme280_compensate_temperature_int32(temp_raw, bme_dev);
  uint32_t compensated_press =
    bme280_compensate_pressure_int64(press_raw, bme_dev);
  uint32_t compensated_hum = bme280_compensate_humidity_int32(hum_raw, bme_dev);

  // Populate readings array based on directory
  for (uint32_t i = 0; i < num_readings; ++i) {
    if (strcmp(bme280_readings_directory.channels[i].name, "temperature") ==
        0) {
      readings[i].type = READING_VALUE_TYPE_FLOAT;
      readings[i].value.float_val = compensated_temp / 100.0f;
    } else if (strcmp(bme280_readings_directory.channels[i].name, "pressure") ==
               0) {
      readings[i].type = READING_VALUE_TYPE_FLOAT;
      readings[i].value.float_val = compensated_press / 256.0f;
    } else if (strcmp(bme280_readings_directory.channels[i].name, "humidity") ==
               0) {
      readings[i].type = READING_VALUE_TYPE_FLOAT;
      readings[i].value.float_val = compensated_hum / 1024.0f;
    } else {
      // Handle unknown channel or error
      return DRIVER_ERROR;
    }
  }

  return DRIVER_OK;
}
