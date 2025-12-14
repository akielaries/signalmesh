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
#include "hal.h" // For I2CDriver

/** @brief Default I2C address for BME280 */
#define BME280_I2C_ADDR 0x77

/** @brief Expected chip ID for BME280 */
#define BME280_CHIP_ID 0x60

/* Register addresses */
/** @brief Chip ID register */
#define BME280_REG_ID 0xD0

/** @brief Humidity control register */
#define BME280_REG_CTRL_HUM     0xF2
/** @brief Status register */
#define BME280_REG_STATUS       0xF3
/** @brief Measurement control register */
#define BME280_REG_CTRL_MEAS    0xF4
/** @brief Configuration register */
#define BME280_REG_CONFIG 0xF5
/** @brief Reset register */
#define BME280_REG_RESET 0xE0
/** @brief Value to write to reset register for soft reset */
#define BME280_SOFT_RESET 0xB6
/** @brief Pressure and temperature data registers */
#define BME280_REG_PRESS_MSB    0xF7
#define BME280_REG_PRESS_LSB    0xF8
#define BME280_REG_PRESS_XLSB   0xF9
#define BME280_REG_TEMP_MSB     0xFA
#define BME280_REG_TEMP_LSB     0xFB
#define BME280_REG_TEMP_XLSB    0xFC
#define BME280_REG_HUM_MSB      0xFD
#define BME280_REG_HUM_LSB      0xFE

// Configuration: Standby time values (for CONFIG register)
#define BME280_CONFIG_T_SB_0_5MS   0b000
#define BME280_CONFIG_T_SB_62_5MS  0b001
#define BME280_CONFIG_T_SB_125MS   0b010
#define BME280_CONFIG_T_SB_250MS   0b011
#define BME280_CONFIG_T_SB_500MS   0b100
#define BME280_CONFIG_T_SB_1000MS  0b101
#define BME280_CONFIG_T_SB_10MS    0b110 // This matches BME280_CONFIG_10_MS
#define BME280_CONFIG_T_SB_20MS    0b111 // This matches BME280_CONFIG_20_MS

// IIR Filter Definitions (for CONFIG register)
#define BME280_FILTER_OFF            0b000
#define BME280_FILTER_X2             0b001
#define BME280_FILTER_X4             0b010
#define BME280_FILTER_X8             0b011
#define BME280_FILTER_X16            0b100

// Oversampling Definitions (for OSRS_T, OSRS_P in CTRL_MEAS, OSRS_H in CTRL_HUM)
#define BME280_OVERSAMPLING_SKIPPED  0b000
#define BME280_OVERSAMPLING_X1       0b001
#define BME280_OVERSAMPLING_X2       0b010
#define BME280_OVERSAMPLING_X4       0b011
#define BME280_OVERSAMPLING_X8       0b100
#define BME280_OVERSAMPLING_X16      0b101

// Mode Definitions (for CTRL_MEAS register)
#define BME280_MODE_SLEEP            0b00
#define BME280_MODE_FORCED           0b01 // or 0b10, same value in spec
#define BME280_MODE_NORMAL           0b11

// Register bit field positions
#define BME280_CTRL_HUM_OSRS_H_POS   0 // Bit 0-2 of CTRL_HUM
#define BME280_CTRL_MEAS_MODE_POS    0 // Bit 0-1 of CTRL_MEAS
#define BME280_CTRL_MEAS_OSRS_P_POS  2 // Bit 2-4 of CTRL_MEAS
#define BME280_CTRL_MEAS_OSRS_T_POS  5 // Bit 5-7 of CTRL_MEAS
#define BME280_CONFIG_FILTER_POS     2 // Bit 2-4 of CONFIG
#define BME280_CONFIG_STANDBY_POS    5 // Bit 5-7 of CONFIG

// Calibration Register Addresses
#define BME280_REG_CALIB_00_25       0x88 // Contains dig_T1-T3, dig_P1-P9
#define BME280_REG_CALIB_26_41       0xE1 // Contains dig_H1-H6

/**
 * @brief BME280 compensation parameters (calibration data)
 */
typedef struct {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;

  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint8_t  dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
} bme280_calib_param_t;

/**
 * @brief BME280 device private data structure
 */
typedef struct {
  bme280_calib_param_t calib_params;
  int32_t  t_fine; // Fine temperature for compensation
  I2CDriver *i2c_driver; // Pointer to the I2C driver instance
  uint8_t   i2c_address;  // I2C address of the device
} bme280_t;

/**
 * @brief External driver instance declaration
 *
 * This is the main driver structure that gets registered with the
 * device registry system.
 */
extern const driver_t bme280_driver;
