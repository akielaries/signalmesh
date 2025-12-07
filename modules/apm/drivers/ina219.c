#include <string.h>
#include <math.h>

#include "bsp/utils/bsp_io.h"

#include "drivers/ina219.h"
#include "ch.h"
#include "hal.h"

// Forward declaration of I2C configuration
extern const I2CConfig bsp_i2c_config;


static uint16_t
ina219_adc_resolution_to_bits(ina219_adc_resolution_t resolution) {
  switch (resolution) {
    case INA219_ADC_9BIT:
      return 0x0;
    case INA219_ADC_10BIT:
      return 0x1;
    case INA219_ADC_11BIT:
      return 0x2;
    case INA219_ADC_12BIT:
      return 0x3;
    default:
      return 0x3;
  }
}

static uint16_t ina219_gain_to_bits(ina219_gain_t gain) {
  switch (gain) {
    case INA219_GAIN_1_40MV:
      return 0x0;
    case INA219_GAIN_2_80MV:
      return 0x1;
    case INA219_GAIN_5_60MV:
      return 0x2;
    case INA219_GAIN_11_20MV:
      return 0x3;
    default:
      return 0x0;
  }
}

static uint16_t ina219_bus_range_to_bits(ina219_bus_range_t range) {
  return (range == INA219_BUS_RANGE_32V) ? INA219_CONFIG_BVOLTAGERANGE_32V
                                         : INA219_CONFIG_BVOLTAGERANGE_16V;
}

static uint16_t ina219_mode_to_bits(ina219_mode_t mode) {
  return (uint16_t)mode;
}

static bool ina219_write_register(ina219_t *dev, uint8_t reg, uint16_t value) {
  uint8_t tx_buf[3];
  tx_buf[0] = reg;
  tx_buf[1] = (value >> 8) & 0xFF; // upper byte
  tx_buf[2] = value & 0xFF;        // lower byte

  msg_t status = i2cMasterTransmitTimeout(&I2CD2,
                                          dev->i2c_address << 1,
                                          tx_buf,
                                          3,
                                          NULL,
                                          0,
                                          TIME_MS2I(1000));
  return status == MSG_OK;
}

static bool ina219_read_register(ina219_t *dev, uint8_t reg, uint16_t *value) {
  uint8_t tx_buf[1] = {reg};
  uint8_t rx_buf[2];

  msg_t status = i2cMasterTransmitTimeout(&I2CD2,
                                          dev->i2c_address << 1,
                                          tx_buf,
                                          1,
                                          rx_buf,
                                          2,
                                          TIME_MS2I(1000));
  if (status == MSG_OK) {
    *value = (rx_buf[0] << 8) | rx_buf[1];
    return true;
  }
  return false;
}

void ina219_init(void) {
  // Configure GPIO pins for I2C2 on PF0/PF1
  // PF0 = I2C2_SDA, PF1 = I2C2_SCL (I2C_B pins)
  // For STM32H7, I2C2 on PF0/PF1 uses AF4
  //palSetPadMode(GPIOF, 0, PAL_MODE_ALTERNATE(4)); // PF0 = I2C2_SDA (AF4)
  //palSetPadMode(GPIOF, 1, PAL_MODE_ALTERNATE(4)); // PF1 = I2C2_SCL (AF4)

  // Start I2C2 driver
  bsp_printf("starting i2c driver...\n");
  chThdSleepMilliseconds(100);
  i2cStart(&I2CD2, &bsp_i2c_config);
  chThdSleepMilliseconds(100);
  bsp_printf("i2c driver started...\n");
}

bool ina219_device_init(ina219_t *dev, const ina219_config_t *config) {
  if (!dev || !config) {
    return false;
  }

  // Initialize device structure
  dev->i2c_address      = config->i2c_address;
  dev->shunt_resistance = config->shunt_resistance;

  // Use calibration similar to reference code for 32V/2A range
  dev->calibration_value = 4096;
  dev->current_lsb       = 0.0001f; // 100uA per bit
  dev->power_lsb         = 0.002f;  // 2mW per bit

  // Reset the device first
  if (!ina219_reset(dev)) {
    return false;
  }

  // Add small delay after reset like reference code
  chThdSleepMilliseconds(1);

  // Write calibration register first
  if (!ina219_write_register(dev,
                             INA219_REG_CALIBRATION,
                             dev->calibration_value)) {
    return false;
  }

  // Configure the device
  return ina219_configure(dev, config);
}

bool ina219_reset(ina219_t *dev) {
  return ina219_write_register(dev, INA219_REG_CONFIG, INA219_CONFIG_RESET);
}

bool ina219_configure(ina219_t *dev, const ina219_config_t *config) {
  uint16_t config_value = 0;

  // Set bus voltage range
  config_value |= ina219_bus_range_to_bits(config->bus_range);

  // Set PGA gain
  config_value |= ina219_gain_to_bits(config->gain);

  // Set bus ADC resolution
  config_value |=
    (ina219_adc_resolution_to_bits(config->bus_adc_resolution) << 7);

  // Set shunt ADC resolution
  config_value |=
    (ina219_adc_resolution_to_bits(config->shunt_adc_resolution) << 3);

  // Set operating mode
  config_value |= ina219_mode_to_bits(config->mode);

  // Write configuration
  bool success = ina219_write_register(dev, INA219_REG_CONFIG, config_value);
  if (!success) {
    return false;
  }

  // Write calibration register
  return ina219_write_register(dev,
                               INA219_REG_CALIBRATION,
                               dev->calibration_value);
}

bool ina219_read_shunt_voltage(ina219_t *dev, float *shunt_voltage_mv) {
  uint16_t raw_value;
  if (!ina219_read_register(dev, INA219_REG_SHUNTVOLTAGE, &raw_value)) {
    return false;
  }

  // Shunt voltage is in 10uV per bit
  *shunt_voltage_mv = ((int16_t)raw_value) * 0.01f;
  return true;
}

bool ina219_read_bus_voltage(ina219_t *dev, float *bus_voltage_v) {
  uint16_t raw_value;
  if (!ina219_read_register(dev, INA219_REG_BUSVOLTAGE, &raw_value)) {
    return false;
  }

  // Bus voltage: ((result >> 3) * 4) - matches reference code
  *bus_voltage_v = ((raw_value >> 3) * 4) / 1000.0f; // Convert mV to V
  return true;
}

bool ina219_read_current(ina219_t *dev, float *current_ma) {
  uint16_t raw_value;
  if (!ina219_read_register(dev, INA219_REG_CURRENT, &raw_value)) {
    return false;
  }

  // Current is in Current_LSB per bit
  *current_ma = ((int16_t)raw_value) * (dev->current_lsb * 1000.0f);
  return true;
}

bool ina219_read_power(ina219_t *dev, float *power_mw) {
  uint16_t raw_value;
  if (!ina219_read_register(dev, INA219_REG_POWER, &raw_value)) {
    return false;
  }

  // Power is in Power_LSB per bit
  *power_mw = raw_value * (dev->power_lsb * 1000.0f);
  return true;
}

bool ina219_read_all(ina219_t *dev,
                     float *shunt_voltage_mv,
                     float *bus_voltage_v,
                     float *current_ma,
                     float *power_mw) {
  return ina219_read_shunt_voltage(dev, shunt_voltage_mv) &&
         ina219_read_bus_voltage(dev, bus_voltage_v) &&
         ina219_read_current(dev, current_ma) &&
         ina219_read_power(dev, power_mw);
}

