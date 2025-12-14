#include <string.h>
#include <math.h>

#include "bsp/utils/bsp_io.h"
#include "drivers/ina219.h"
#include "ch.h"
#include "hal.h"
#include "drivers/driver_readings.h"
#include "drivers/i2c.h"

// Macro to calculate the size of an array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Static helper functions prototypes
static bool ina219_device_init_helper(ina219_t *dev_data, const ina219_config_t *config);
static bool ina219_read_all_helper(ina219_t *dev_data,
                                   float *shunt_voltage_mv,
                                   float *bus_voltage_v,
                                   float *current_ma,
                                   float *power_mw);
static bool ina219_configure_helper(ina219_t *dev_data, const ina219_config_t *config);

static int ina219_init(device_t *dev);
static int ina219_remove(device_t *dev);
static int ina219_ioctl(device_t *dev, uint32_t cmd, void *arg);
static int ina219_poll(device_id_t device_id, uint32_t num_readings, driver_reading_t *readings);

static int ina219_read_register(ina219_t *dev, uint8_t reg, uint16_t *val) {
  uint8_t buf[2];
  int ret = i2c_bus_read_reg(&dev->bus, reg, buf, 2);
  if (ret != 0) {
    return ret;
  }
  *val = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
  return 0;
}

static int ina219_write_register(ina219_t *dev, uint8_t reg, uint16_t val) {
  uint8_t buf[2];
  buf[0] = (uint8_t)(val >> 8);
  buf[1] = (uint8_t)(val & 0xFF);
  return i2c_bus_write_reg(&dev->bus, reg, buf, 2);
}


// INA219 specific reading channels
static const driver_reading_channel_t ina219_reading_channels[] = {
  {
    .channel_type = READING_CHANNEL_TYPE_VOLTAGE,
    .name         = "shunt_voltage",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_VOLTAGE,
    .name         = "bus_voltage",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_CURRENT,
    .name         = "current",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_POWER,
    .name         = "power",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
};

// INA219 readings directory
static const driver_readings_directory_t ina219_readings_directory = {
  .num_readings = ARRAY_SIZE(ina219_reading_channels),
  .channels     = ina219_reading_channels,
};

const driver_t ina219_driver __attribute__((used)) = {
  .name               = "ina219",
  .init               = ina219_init,
  .remove             = ina219_remove,
  .ioctl              = ina219_ioctl,
  .poll               = ina219_poll,
  .readings_directory = &ina219_readings_directory,
};

static int ina219_init(device_t *dev) {
  if (dev->priv == NULL) {
    return DRIVER_ERROR; // Private data not allocated
  }
  ina219_t *ina_dev = (ina219_t *)dev->priv;
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

  ina_dev->bus.i2c  = (I2CDriver *)dev->bus;
  ina_dev->bus.addr = config.i2c_address;

  if (!ina219_device_init_helper(ina_dev, &config)) {
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

static int ina219_remove(device_t *dev) {
  (void)dev;
  // Add removal logic
  return DRIVER_OK;
}

static int ina219_ioctl(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;
  (void)cmd;
  (void)arg;
  return DRIVER_NOT_FOUND;
}

static int ina219_poll(device_id_t device_id, uint32_t num_readings, driver_reading_t *readings) {
  if (device_id == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > ina219_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM; // Caller asked for more readings than available
  }

  ina219_t *ina_dev_data = (ina219_t *)device_id->priv;
  float shunt_v, bus_v, current_ma, power_mw;

  if (!ina219_read_all_helper(ina_dev_data, &shunt_v, &bus_v, &current_ma, &power_mw)) {
    return DRIVER_ERROR;
  }

  // Populate readings array
  for (uint32_t i = 0; i < num_readings; ++i) {
    switch (ina219_readings_directory.channels[i].channel_type) {
      case READING_CHANNEL_TYPE_VOLTAGE:
        if (strcmp(ina219_readings_directory.channels[i].name, "shunt_voltage") == 0) {
          readings[i].type            = READING_VALUE_TYPE_FLOAT;
          readings[i].value.float_val = shunt_v;
        } else if (strcmp(ina219_readings_directory.channels[i].name, "bus_voltage") == 0) {
          readings[i].type            = READING_VALUE_TYPE_FLOAT;
          readings[i].value.float_val = bus_v;
        }
        break;
      case READING_CHANNEL_TYPE_CURRENT:
        readings[i].type            = READING_VALUE_TYPE_FLOAT;
        readings[i].value.float_val = current_ma;
        break;
      case READING_CHANNEL_TYPE_POWER:
        readings[i].type            = READING_VALUE_TYPE_FLOAT;
        readings[i].value.float_val = power_mw;
        break;
      default:
        return DRIVER_ERROR;
    }
  }

  return DRIVER_OK;
}

static uint16_t ina219_adc_resolution_to_bits_helper(ina219_adc_resolution_t resolution) {
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

static uint16_t ina219_mode_to_bits_helper(ina219_mode_t mode) { return (uint16_t)mode; }

static bool ina219_device_init_helper(ina219_t *dev_data, const ina219_config_t *config) {
  if (!dev_data || !config) {
    return false;
  }

  // Initialize device structure
  dev_data->bus.addr         = config->i2c_address;
  dev_data->shunt_resistance = config->shunt_resistance;

  // Use calibration similar to reference code for 32V/2A range
  dev_data->calibration_value = 4096;
  dev_data->current_lsb       = 0.0001f; // 100uA per bit
  dev_data->power_lsb         = 0.002f;  // 2mW per bit

  uint16_t val;
  if (ina219_read_register(dev_data, 0x00, &val) == 0) {
    return true;
  } else {
    return false;
  }

  // Write calibration register first
  if (ina219_write_register(dev_data, INA219_REG_CALIBRATION, dev_data->calibration_value) != 0) {
    return false;
  }

  // Configure the device
  if (!ina219_configure_helper(dev_data, config)) {
    return false;
  }
  return true;
}

static bool ina219_reset_helper(ina219_t *dev_data) {
  return ina219_write_register(dev_data, INA219_REG_CONFIG, INA219_CONFIG_RESET) == 0;
}

static bool ina219_configure_helper(ina219_t *dev_data, const ina219_config_t *config) {
  uint16_t config_value = 0;

  // Set bus voltage range
  config_value |= ina219_bus_range_to_bits_helper(config->bus_range);

  // Set PGA gain
  config_value |= ina219_gain_to_bits_helper(config->gain);

  // Set bus ADC resolution
  config_value |= (ina219_adc_resolution_to_bits_helper(config->bus_adc_resolution) << 7);

  // Set shunt ADC resolution
  config_value |= (ina219_adc_resolution_to_bits_helper(config->shunt_adc_resolution) << 3);

  // Set operating mode
  config_value |= ina219_mode_to_bits_helper(config->mode);

  // Write configuration
  if (ina219_write_register(dev_data, INA219_REG_CONFIG, config_value) != 0) {
    return false;
  }

  // Write calibration register
  return ina219_write_register(dev_data, INA219_REG_CALIBRATION, dev_data->calibration_value) == 0;
}

static bool ina219_read_shunt_voltage_helper(ina219_t *dev_data, float *shunt_voltage_mv) {
  uint16_t raw_value;
  if (ina219_read_register(dev_data, INA219_REG_SHUNTVOLTAGE, &raw_value) != 0) {
    return false;
  }

  // Shunt voltage is in 10uV per bit
  *shunt_voltage_mv = ((int16_t)raw_value) * 0.01f;
  return true;
}

static bool ina219_read_bus_voltage_helper(ina219_t *dev_data, float *bus_voltage_v) {
  uint16_t raw_value;
  if (ina219_read_register(dev_data, INA219_REG_BUSVOLTAGE, &raw_value) != 0) {
    return false;
  }

  // Bus voltage: ((raw_value >> 3) * 4) - matches reference code
  *bus_voltage_v = ((raw_value >> 3) * 4) / 1000.0f; // Convert mV to V
  return true;
}

static bool ina219_read_current_helper(ina219_t *dev_data, float *current_ma) {
  uint16_t raw_value;
  if (ina219_read_register(dev_data, INA219_REG_CURRENT, &raw_value) != 0) {
    return false;
  }

  // Current is in Current_LSB per bit
  *current_ma = ((int16_t)raw_value) * (dev_data->current_lsb * 1000.0f);
  return true;
}

static bool ina219_read_power_helper(ina219_t *dev_data, float *power_mw) {
  uint16_t raw_value;
  if (ina219_read_register(dev_data, INA219_REG_POWER, &raw_value) != 0) {
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
