#include <string.h>
#include <math.h>

#include "bsp/utils/bsp_io.h"

#include "drivers/ina219.h"
#include "ch.h"
#include "hal.h"

// Static helper functions
static void handle_i2c_err_helper(I2CDriver *i2c_driver, const char *context);
static uint16_t
ina219_adc_resolution_to_bits_helper(ina219_adc_resolution_t resolution);
static uint16_t ina219_gain_to_bits_helper(ina219_gain_t gain);
static uint16_t ina219_bus_range_to_bits_helper(ina219_bus_range_t range);
static uint16_t ina219_mode_to_bits_helper(ina219_mode_t mode);
static bool
ina219_write_register_helper(ina219_t *dev_data, uint8_t reg, uint16_t value);
static bool
ina219_read_register_helper(ina219_t *dev_data, uint8_t reg, uint16_t *value);
static bool ina219_device_init_helper(ina219_t *dev_data,
                                      const ina219_config_t *config);
static bool ina219_reset_helper(ina219_t *dev_data);
static bool ina219_configure_helper(ina219_t *dev_data,
                                    const ina219_config_t *config);
static bool ina219_read_shunt_voltage_helper(ina219_t *dev_data,
                                             float *shunt_voltage_mv);
static bool ina219_read_bus_voltage_helper(ina219_t *dev_data,
                                           float *bus_voltage_v);
static bool ina219_read_current_helper(ina219_t *dev_data, float *current_ma);
static bool ina219_read_power_helper(ina219_t *dev_data, float *power_mw);
static bool ina219_read_all_helper(ina219_t *dev_data,
                                   float *shunt_voltage_mv,
                                   float *bus_voltage_v,
                                   float *current_ma,
                                   float *power_mw);


static int ina219_init_fn(device_t *dev) {
  if (dev->priv == NULL) {
    return DRIVER_ERROR; // Private data not allocated
  }
  ina219_t *ina_dev_data = (ina219_t *)dev->priv;
  // Assuming config comes from somewhere, for now hardcoding
  ina219_config_t config = {
    .i2c_address          = INA219_DEFAULT_ADDRESS,
    .shunt_resistance     = 0.1f, // Example value
    .bus_range            = INA219_BUS_RANGE_32V,
    .gain                 = INA219_GAIN_1_40MV,
    .bus_adc_resolution   = INA219_ADC_12BIT,
    .shunt_adc_resolution = INA219_ADC_12BIT,
    .mode                 = INA219_MODE_SHUNT_BUS_CONT,
  };

  if (!ina219_device_init_helper(ina_dev_data, &config)) {
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

static int ina219_probe_fn(device_t *dev) {
  (void)dev;
  // Probe logic, currently handled by ina219_device_init_helper
  return DRIVER_OK;
}

static int ina219_remove_fn(device_t *dev) {
  (void)dev;
  // Add removal logic
  return DRIVER_OK;
}

static int ina219_ioctl_fn(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;
  (void)cmd;
  (void)arg;
  return DRIVER_NOT_FOUND;
}

static int ina219_poll_fn(device_t *dev) {
  // Add polling logic
  (void)dev;
  // Example: Read current and voltage
  float shunt_v, bus_v, current_ma, power_mw;
  ina219_t *ina_dev_data = (ina219_t *)dev->priv;
  if (ina219_read_all_helper(ina_dev_data,
                             &shunt_v,
                             &bus_v,
                             &current_ma,
                             &power_mw)) {
    bsp_printf("INA219 (%s) - Shunt: %.2fmV, Bus: %.2fV, Current: %.2fmA, "
               "Power: %.2fmW\n",
               dev->name,
               shunt_v,
               bus_v,
               current_ma,
               power_mw);
    return DRIVER_OK;
  }
  return DRIVER_ERROR;
}


const driver_t ina219_driver __attribute__((used)) = {
  .name   = "ina219",
  .init   = ina219_init_fn,
  .probe  = ina219_probe_fn,
  .remove = ina219_remove_fn,
  .ioctl  = ina219_ioctl_fn,
  .poll   = ina219_poll_fn,
};

static void handle_i2c_err_helper(I2CDriver *i2c_driver, const char *context) {
  i2cflags_t err = i2cGetErrors(i2c_driver);
  if (err != I2C_NO_ERROR) {
    bsp_printf("INA219 I2C error in %s: ", context);
    if (err & I2C_BUS_ERROR) {
      bsp_printf("BUS ERROR\n");
    } else if (err & I2C_ARBITRATION_LOST) {
      bsp_printf("ARBITRATION LOST\n");
    } else if (err & I2C_ACK_FAILURE) {
      bsp_printf("I2C ACK FAILURE\n");
    } else if (err & I2C_OVERRUN) {
      bsp_printf("I2C OVERRUN\n");
    } else {
      bsp_printf("unknown error code: 0x%02X\n", err);
    }
  }

  // i2cStop(i2c_driver);
  // i2cStart(i2c_driver, i2c_driver->config);
  // bsp_printf("I2C bus recovered (%s)\n", context);
}


static uint16_t
ina219_adc_resolution_to_bits_helper(ina219_adc_resolution_t resolution) {
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

static uint16_t ina219_gain_to_bits_helper(ina219_gain_t gain) {
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

static uint16_t ina219_bus_range_to_bits_helper(ina219_bus_range_t range) {
  return (range == INA219_BUS_RANGE_32V) ? INA219_CONFIG_BVOLTAGERANGE_32V
                                         : INA219_CONFIG_BVOLTAGERANGE_16V;
}

static uint16_t ina219_mode_to_bits_helper(ina219_mode_t mode) {
  return (uint16_t)mode;
}

static bool
ina219_write_register_helper(ina219_t *dev_data, uint8_t reg, uint16_t value) {
  uint8_t tx[3];
  tx[0] = reg;
  tx[1] = (value >> 8);
  tx[2] = (value & 0xFF);

  // i2cAcquireBus(&I2CD4);
  msg_t msg = i2cMasterTransmitTimeout(&I2CD4,
                                       dev_data->i2c_address,
                                       tx,
                                       3,
                                       NULL,
                                       0,
                                       TIME_MS2I(100));
  // i2cReleaseBus(&I2CD4);
  if (msg != MSG_OK) {
    handle_i2c_err_helper(&I2CD4, "ina219_write_register");
    return false;
  }

  return true;
}

static bool
ina219_read_register_helper(ina219_t *dev_data, uint8_t reg, uint16_t *value) {
  uint8_t tx = reg;
  uint8_t rx[2];

  msg_t msg = i2cMasterTransmitTimeout(&I2CD4,
                                       dev_data->i2c_address,
                                       &tx,
                                       1,
                                       rx,
                                       2,
                                       TIME_MS2I(100));

  if (msg != MSG_OK) {
    bsp_printf("read error: 0x%X\n", msg);
    handle_i2c_err_helper(&I2CD4, "ina219_read_register");
    return false;
  }

  // INA219 uses big-endian registers
  *value = ((uint16_t)rx[0] << 8) | rx[1];

  return true;
}

static bool ina219_device_init_helper(ina219_t *dev_data,
                                      const ina219_config_t *config) {
  if (!dev_data || !config) {
    return false;
  }

  // Initialize device structure
  dev_data->i2c_address      = config->i2c_address;
  dev_data->shunt_resistance = config->shunt_resistance;

  // Use calibration similar to reference code for 32V/2A range
  dev_data->calibration_value = 4096;
  dev_data->current_lsb       = 0.0001f; // 100uA per bit
  dev_data->power_lsb         = 0.002f;  // 2mW per bit
  bsp_printf("INA219 using address 0x%02X\n", dev_data->i2c_address);

  /*
    // Reset the device first
    bsp_printf("Resetting INA219...\n");
    if (!ina219_reset_helper(dev_data)) {
      bsp_printf("Failed to reset INA219\n");
      return false;
    }

    // Add small delay after reset like reference code
    chThdSleepMilliseconds(1);
  */
  uint16_t val;
  if (ina219_read_register_helper(dev_data, 0x00, &val)) {
    bsp_printf("Reg0 = 0x%04X\n", val);
    return true;
  } else {
    bsp_printf("Reg0 read failed\n");
    return false;
  }

  // Write calibration register first
  bsp_printf("Writing INA219 calibration...\n");
  if (!ina219_write_register_helper(dev_data,
                                    INA219_REG_CALIBRATION,
                                    dev_data->calibration_value)) {
    bsp_printf("Failed to write INA219 calibration\n");
    return false;
  }

  // Configure the device
  bsp_printf("Configuring INA219...\n");
  if (!ina219_configure_helper(dev_data, config)) {
    bsp_printf("Failed to configure INA219\n");
    return false;
  }
  return true;
}

static bool ina219_reset_helper(ina219_t *dev_data) {
  return ina219_write_register_helper(dev_data,
                                      INA219_REG_CONFIG,
                                      INA219_CONFIG_RESET);
}

static bool ina219_configure_helper(ina219_t *dev_data,
                                    const ina219_config_t *config) {
  uint16_t config_value = 0;

  // Set bus voltage range
  config_value |= ina219_bus_range_to_bits_helper(config->bus_range);

  // Set PGA gain
  config_value |= ina219_gain_to_bits_helper(config->gain);

  // Set bus ADC resolution
  config_value |=
    (ina219_adc_resolution_to_bits_helper(config->bus_adc_resolution) << 7);

  // Set shunt ADC resolution
  config_value |=
    (ina219_adc_resolution_to_bits_helper(config->shunt_adc_resolution) << 3);

  // Set operating mode
  config_value |= ina219_mode_to_bits_helper(config->mode);

  // Write configuration
  bool success =
    ina219_write_register_helper(dev_data, INA219_REG_CONFIG, config_value);
  if (!success) {
    return false;
  }

  // Write calibration register
  return ina219_write_register_helper(dev_data,
                                      INA219_REG_CALIBRATION,
                                      dev_data->calibration_value);
}

static bool ina219_read_shunt_voltage_helper(ina219_t *dev_data,
                                             float *shunt_voltage_mv) {
  uint16_t raw_value;
  if (!ina219_read_register_helper(dev_data,
                                   INA219_REG_SHUNTVOLTAGE,
                                   &raw_value)) {
    return false;
  }

  // Shunt voltage is in 10uV per bit
  *shunt_voltage_mv = ((int16_t)raw_value) * 0.01f;
  return true;
}

static bool ina219_read_bus_voltage_helper(ina219_t *dev_data,
                                           float *bus_voltage_v) {
  uint16_t raw_value;
  if (!ina219_read_register_helper(dev_data,
                                   INA219_REG_BUSVOLTAGE,
                                   &raw_value)) {
    return false;
  }

  // Bus voltage: ((raw_value >> 3) * 4) - matches reference code
  *bus_voltage_v = ((raw_value >> 3) * 4) / 1000.0f; // Convert mV to V
  return true;
}

static bool ina219_read_current_helper(ina219_t *dev_data, float *current_ma) {
  uint16_t raw_value;
  if (!ina219_read_register_helper(dev_data, INA219_REG_CURRENT, &raw_value)) {
    return false;
  }

  // Current is in Current_LSB per bit
  *current_ma = ((int16_t)raw_value) * (dev_data->current_lsb * 1000.0f);
  return true;
}

static bool ina219_read_power_helper(ina219_t *dev_data, float *power_mw) {
  uint16_t raw_value;
  if (!ina219_read_register_helper(dev_data, INA219_REG_POWER, &raw_value)) {
    return false;
  }

  // Power is in Power_LSB per bit
  *power_mw = raw_value * (dev_data->power_lsb * 1000.0f);
  return true;
}

static bool ina219_read_all_helper(ina219_t *dev_data,
                                   float *shunt_voltage_mv,
                                   float *bus_voltage_v,
                                   float *current_ma,
                                   float *power_mw) {
  return ina219_read_shunt_voltage_helper(dev_data, shunt_voltage_mv) &&
         ina219_read_bus_voltage_helper(dev_data, bus_voltage_v) &&
         ina219_read_current_helper(dev_data, current_ma) &&
         ina219_read_power_helper(dev_data, power_mw);
}
