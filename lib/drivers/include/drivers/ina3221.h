#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"
#include "drivers/i2c.h"


#ifdef __cplusplus
extern "C" {
#endif

// INA3221 I2C assuming VS and GND are tied together. default is 0x40
#define INA3221_DEFAULT_ADDRESS 0x41

// INA3221 register addresses
#define INA3221_REG_CONFIG       0x00
#define INA3221_REG_SHUNTVOLTAGE 0x01
#define INA3221_REG_BUSVOLTAGE   0x02
#define INA3221_REG_POWER        0x03
#define INA3221_REG_CURRENT      0x04
#define INA3221_REG_CALIBRATION  0x05

// Configuration register bit fields
#define INA3221_CONFIG_RESET               0x8000

#define INA3221_CONFIG_BVOLTAGERANGE_MASK  0x2000
#define INA3221_CONFIG_BVOLTAGERANGE_16V   0x0000
#define INA3221_CONFIG_BVOLTAGERANGE_32V   0x2000

#define INA3221_CONFIG_GAIN_MASK           0x1800
#define INA3221_CONFIG_GAIN_1_40MV         0x0000
#define INA3221_CONFIG_GAIN_2_80MV         0x0800
#define INA3221_CONFIG_GAIN_4_160MV        0x1000
#define INA3221_CONFIG_GAIN_8_320MV        0x1800

#define INA3221_CONFIG_BADCRES_MASK        0x0780

#define INA3221_CONFIG_BADCRES_9BIT        0x0080
#define INA3221_CONFIG_BADCRES_10BIT       0x0100
#define INA3221_CONFIG_BADCRES_11BIT       0x0200
#define INA3221_CONFIG_BADCRES_12BIT       0x0400

#define INA3221_CONFIG_ADC_9_BIT_1_SAMPLES   0x0
#define INA3221_CONFIG_ADC_10_BIT_1_SAMPLES  0x1
#define INA3221_CONFIG_ADC_11_BIT_1_SAMPLES  0x2

#define INA3221_CONFIG_ADC_12_BIT_1_SAMPLES  0x3
#define INA3221_CONFIG_ADC_12_BIT_2_SAMPLES  0x9
#define INA3221_CONFIG_ADC_12_BIT_4_SAMPLES  0xA
#define INA3221_CONFIG_ADC_12_BIT_8_SAMPLES  0xB
#define INA3221_CONFIG_ADC_12_BIT_16_SAMPLES 0xC
#define INA3221_CONFIG_ADC_12_BIT_32_SAMPLES 0xD
#define INA3221_CONFIG_ADC_12_BIT_64_SAMPLES 0xE
#define INA3221_CONFIG_ADC_12_BIT_128_SAMPLES 0xF

#define INA3221_CONFIG_SADCRES_MASK        0x0078

#define INA3221_CONFIG_SADCRES_9BIT        0x0008
#define INA3221_CONFIG_SADCRES_10BIT       0x0010
#define INA3221_CONFIG_SADCRES_11BIT       0x0020
#define INA3221_CONFIG_SADCRES_12BIT       0x0040


#define INA3221_CONFIG_BADCRES_SHIFT  7  // Bus ADC resolution bits 10-7
#define INA3221_CONFIG_SADCRES_SHIFT  3  // Shunt ADC resolution bits 6-3


#define INA3221_CONFIG_MODE_MASK           0x0007

#define INA3221_CONFIG_MODE_POWERDOWN      0x0000
#define INA3221_CONFIG_MODE_SVOLT_TRIG     0x0001
#define INA3221_CONFIG_MODE_BVOLT_TRIG     0x0002
#define INA3221_CONFIG_MODE_SANDBVOLT_TRIG 0x0003
#define INA3221_CONFIG_MODE_ADCOFF         0x0004
#define INA3221_CONFIG_MODE_SVOLT_CONT     0x0005
#define INA3221_CONFIG_MODE_BVOLT_CONT     0x0006
#define INA3221_CONFIG_MODE_SANDBVOLT_CONT 0x0007


// Bus voltage range
typedef enum {
  INA3221_BUS_RANGE_16V = 0x0,
  INA3221_BUS_RANGE_32V = 0x1,
} ina3221_bus_range_t;

// PGA gain settings
typedef enum {
  INA3221_GAIN_1_40MV = 0x0,
  INA3221_GAIN_2_80MV = 0x1,
  INA3221_GAIN_4_160MV = 0x2,
  INA3221_GAIN_8_320MV = 0x3
} ina3221_gain_t;

// ADC resolution settings
typedef enum {
  INA3221_ADC_9BIT,
  INA3221_ADC_10BIT,
  INA3221_ADC_11BIT,
  INA3221_ADC_12BIT
} ina3221_adc_resolution_t;

// Operating mode
typedef enum {
  INA3221_MODE_POWERDOWN,
  INA3221_MODE_SHUNT_TRIG,
  INA3221_MODE_BUS_TRIG,
  INA3221_MODE_SHUNT_BUS_TRIG,
  INA3221_MODE_ADCOFF,
  INA3221_MODE_SHUNT_CONT,
  INA3221_MODE_BUS_CONT,
  INA3221_MODE_SHUNT_BUS_CONT
} ina3221_mode_t;

// INA3221 configuration structure
typedef struct {
  uint8_t i2c_address;
  float shunt_resistance;
  ina3221_bus_range_t bus_range;
  ina3221_gain_t gain;
  ina3221_adc_resolution_t bus_adc_resolution;
  ina3221_adc_resolution_t shunt_adc_resolution;
  ina3221_mode_t mode;
} ina3221_config_t;

// INA3221 device structure
typedef struct {
  i2c_bus_t bus;
  float shunt_resistance;
  float current_lsb;
  float power_lsb;
  uint16_t calibration_value;
} ina3221_t;


/**
 * @brief Initialize an INA3221 device with the given configuration.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param config Pointer to the configuration structure.
 * @return true if initialization successful, false otherwise.
 */
bool ina3221_device_init(ina3221_t *dev, const ina3221_config_t *config);

/**
 * @brief Reset the INA3221 device.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @return true if reset successful, false otherwise.
 */
bool ina3221_reset(ina3221_t *dev);

/**
 * @brief Configure the INA3221 device.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param config Pointer to the configuration structure.
 * @return true if configuration successful, false otherwise.
 */
bool ina3221_configure(ina3221_t *dev, const ina3221_config_t *config);

/**
 * @brief Read the shunt voltage in millivolts.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param[out] shunt_voltage_mv Pointer to store the shunt voltage in mV.
 * @return true if read successful, false otherwise.
 */
bool ina3221_read_shunt_voltage(ina3221_t *dev, float *shunt_voltage_mv);

/**
 * @brief Read the bus voltage in volts.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param[out] bus_voltage_v Pointer to store the bus voltage in V.
 * @return true if read successful, false otherwise.
 */
bool ina3221_read_bus_voltage(ina3221_t *dev, float *bus_voltage_v);

/**
 * @brief Read the current in milliamps.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param[out] current_ma Pointer to store the current in mA.
 * @return true if read successful, false otherwise.
 */
bool ina3221_read_current(ina3221_t *dev, float *current_ma);

/**
 * @brief Read the power in milliwatts.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param[out] power_mw Pointer to store the power in mW.
 * @return true if read successful, false otherwise.
 */
bool ina3221_read_power(ina3221_t *dev, float *power_mw);

/**
 * @brief Read all measurements at once.
 *
 * @param dev Pointer to the INA3221 device structure.
 * @param[out] shunt_voltage_mv Pointer to store the shunt voltage in mV.
 * @param[out] bus_voltage_v Pointer to store the bus voltage in V.
 * @param[out] current_ma Pointer to store the current in mA.
 * @param[out] power_mw Pointer to store the power in mW.
 * @return true if read successful, false otherwise.
 */
bool ina3221_read_all(ina3221_t *dev,
                     float *shunt_voltage_mv,
                     float *bus_voltage_v,
                     float *current_ma,
                     float *power_mw);

extern const driver_t ina3221_driver;

#ifdef __cplusplus
}
#endif
