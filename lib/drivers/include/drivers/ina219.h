#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// INA219 I2C address (default address with A0, A1 pins grounded)
#define INA219_DEFAULT_ADDRESS 0x40

// INA219 register addresses
#define INA219_REG_CONFIG       0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE   0x02
#define INA219_REG_POWER        0x03
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

// Configuration register bit fields
#define INA219_CONFIG_RESET               0x8000
#define INA219_CONFIG_BVOLTAGERANGE_MASK  0x2000
#define INA219_CONFIG_BVOLTAGERANGE_16V   0x0000
#define INA219_CONFIG_BVOLTAGERANGE_32V   0x2000
#define INA219_CONFIG_GAIN_MASK           0x1800
#define INA219_CONFIG_GAIN_1_40MV         0x0000
#define INA219_CONFIG_GAIN_2_80MV         0x0800
#define INA219_CONFIG_GAIN_5_60MV         0x1000
#define INA219_CONFIG_GAIN_11_20MV        0x1800
#define INA219_CONFIG_BADCRES_MASK        0x0780
#define INA219_CONFIG_BADCRES_9BIT        0x0080
#define INA219_CONFIG_BADCRES_10BIT       0x0100
#define INA219_CONFIG_BADCRES_11BIT       0x0200
#define INA219_CONFIG_BADCRES_12BIT       0x0400
#define INA219_CONFIG_SADCRES_MASK        0x0078
#define INA219_CONFIG_SADCRES_9BIT        0x0008
#define INA219_CONFIG_SADCRES_10BIT       0x0010
#define INA219_CONFIG_SADCRES_11BIT       0x0020
#define INA219_CONFIG_SADCRES_12BIT       0x0040
#define INA219_CONFIG_MODE_MASK           0x0007
#define INA219_CONFIG_MODE_POWERDOWN      0x0000
#define INA219_CONFIG_MODE_SVOLT_TRIG     0x0001
#define INA219_CONFIG_MODE_BVOLT_TRIG     0x0002
#define INA219_CONFIG_MODE_SANDBVOLT_TRIG 0x0003
#define INA219_CONFIG_MODE_ADCOFF         0x0004
#define INA219_CONFIG_MODE_SVOLT_CONT     0x0005
#define INA219_CONFIG_MODE_BVOLT_CONT     0x0006
#define INA219_CONFIG_MODE_SANDBVOLT_CONT 0x0007

// Bus voltage range
typedef enum { INA219_BUS_RANGE_16V, INA219_BUS_RANGE_32V } ina219_bus_range_t;

// PGA gain settings
typedef enum {
  INA219_GAIN_1_40MV,
  INA219_GAIN_2_80MV,
  INA219_GAIN_5_60MV,
  INA219_GAIN_11_20MV
} ina219_gain_t;

// ADC resolution settings
typedef enum {
  INA219_ADC_9BIT,
  INA219_ADC_10BIT,
  INA219_ADC_11BIT,
  INA219_ADC_12BIT
} ina219_adc_resolution_t;

// Operating mode
typedef enum {
  INA219_MODE_POWERDOWN,
  INA219_MODE_SHUNT_TRIG,
  INA219_MODE_BUS_TRIG,
  INA219_MODE_SHUNT_BUS_TRIG,
  INA219_MODE_ADCOFF,
  INA219_MODE_SHUNT_CONT,
  INA219_MODE_BUS_CONT,
  INA219_MODE_SHUNT_BUS_CONT
} ina219_mode_t;

// INA219 configuration structure
typedef struct {
  uint8_t i2c_address;
  float shunt_resistance;
  ina219_bus_range_t bus_range;
  ina219_gain_t gain;
  ina219_adc_resolution_t bus_adc_resolution;
  ina219_adc_resolution_t shunt_adc_resolution;
  ina219_mode_t mode;
} ina219_config_t;

// INA219 device structure
typedef struct {
  uint8_t i2c_address;
  float shunt_resistance;
  float current_lsb;
  float power_lsb;
  uint16_t calibration_value;
} ina219_t;



/**
 * @brief Initialize an INA219 device with the given configuration.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param config Pointer to the configuration structure.
 * @return true if initialization successful, false otherwise.
 */
bool ina219_device_init(ina219_t *dev, const ina219_config_t *config);

/**
 * @brief Reset the INA219 device.
 *
 * @param dev Pointer to the INA219 device structure.
 * @return true if reset successful, false otherwise.
 */
bool ina219_reset(ina219_t *dev);

/**
 * @brief Configure the INA219 device.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param config Pointer to the configuration structure.
 * @return true if configuration successful, false otherwise.
 */
bool ina219_configure(ina219_t *dev, const ina219_config_t *config);

/**
 * @brief Read the shunt voltage in millivolts.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param[out] shunt_voltage_mv Pointer to store the shunt voltage in mV.
 * @return true if read successful, false otherwise.
 */
bool ina219_read_shunt_voltage(ina219_t *dev, float *shunt_voltage_mv);

/**
 * @brief Read the bus voltage in volts.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param[out] bus_voltage_v Pointer to store the bus voltage in V.
 * @return true if read successful, false otherwise.
 */
bool ina219_read_bus_voltage(ina219_t *dev, float *bus_voltage_v);

/**
 * @brief Read the current in milliamps.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param[out] current_ma Pointer to store the current in mA.
 * @return true if read successful, false otherwise.
 */
bool ina219_read_current(ina219_t *dev, float *current_ma);

/**
 * @brief Read the power in milliwatts.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param[out] power_mw Pointer to store the power in mW.
 * @return true if read successful, false otherwise.
 */
bool ina219_read_power(ina219_t *dev, float *power_mw);

/**
 * @brief Read all measurements at once.
 *
 * @param dev Pointer to the INA219 device structure.
 * @param[out] shunt_voltage_mv Pointer to store the shunt voltage in mV.
 * @param[out] bus_voltage_v Pointer to store the bus voltage in V.
 * @param[out] current_ma Pointer to store the current in mA.
 * @param[out] power_mw Pointer to store the power in mW.
 * @return true if read successful, false otherwise.
 */
bool ina219_read_all(ina219_t *dev,
                     float *shunt_voltage_mv,
                     float *bus_voltage_v,
                     float *current_ma,
                     float *power_mw);

extern const driver_t ina219_driver;

#ifdef __cplusplus
}
#endif
