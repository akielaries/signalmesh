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
static bool ina219_read_all_helper(ina219_t *dev_data,
                                   float *shunt_voltage_mv,
                                   float *bus_voltage_v,
                                   float *current_ma,
                                   float *power_mw);

static int ina219_init(device_t *dev);
static int ina219_remove(device_t *dev);
static int ina219_ioctl(device_t *dev, uint32_t cmd, void *arg);
static int ina219_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings);

static int ina219_read_register(ina219_t *dev, uint8_t reg, uint16_t *val) {
  uint8_t buf[2];
  int ret = i2c_bus_read_reg(&dev->bus, reg, buf, 2);
  if (ret != 0) {
    bsp_printf("INA219 read_reg failed: reg=0x%02X, err=%d\n", reg, ret);
    return ret;
  }
  *val = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
  return 0;
}

static int ina219_write_register(ina219_t *dev, uint8_t reg, uint16_t val) {
  uint8_t buf[2];
  buf[0] = (uint8_t)(val >> 8);
  buf[1] = (uint8_t)(val & 0xFF);
  int ret = i2c_bus_write_reg(&dev->bus, reg, buf, 2);
  if (ret != 0) {
      bsp_printf("INA219 write_reg failed: reg=0x%02X, err=%d\n", reg, ret);
  }
  return ret;
}


// INA219 specific reading channels
static const driver_reading_channel_t ina219_reading_channels[] = {
  {
    .channel_type = READING_CHANNEL_TYPE_VOLTAGE,
    .name         = "shunt_voltage",
    .unit         = "mV",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_VOLTAGE,
    .name         = "bus_voltage",
    .unit         = "V",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_CURRENT,
    .name         = "current",
    .unit         = "mA",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_POWER,
    .name         = "power",
    .unit         = "mW",
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
  .poll               = (int (*)(device_id_t, uint32_t, driver_reading_t *))ina219_poll,
  .readings_directory = &ina219_readings_directory,
};
static int ina219_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("INA219 init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  ina219_t *ina_dev = (ina219_t *)dev->priv;

  bsp_printf("INA219: Initializing...\n");

  ina_dev->bus.i2c  = (I2CDriver *)dev->bus;
  ina_dev->bus.addr = INA219_DEFAULT_ADDRESS;

  bsp_printf("INA219: Resetting device...\n");
  if (ina219_write_register(ina_dev, INA219_REG_CONFIG, INA219_CONFIG_RESET) != 0) {
    bsp_printf("INA219: Failed to reset device.\n");
    return DRIVER_ERROR;
  }
  chThdSleepMilliseconds(5);

  // Build configuration register value
  uint16_t config_value = 0;
  
  // Bus voltage range: 32V (bit 13 = 1)
  config_value |= (1 << 13);
  
  // PGA gain: /8 for ±320mV range (bits 12-11 = 11)
  // This gives us better range for higher currents
  config_value |= (3 << 11);
  
  // Bus ADC: 12-bit, 1 sample (bits 10-7 = 0x3)
  config_value |= (0x3 << 7);
  
  // Shunt ADC: 12-bit, 1 sample (bits 6-3 = 0x3)
  config_value |= (0x3 << 3);
  
  // Mode: Shunt and bus voltage continuous (bits 2-0 = 0x7)
  config_value |= 0x7;
  
  bsp_printf("INA219: Writing config value: 0x%04X\n", config_value);
  if (ina219_write_register(ina_dev, INA219_REG_CONFIG, config_value) != 0) {
    bsp_printf("INA219: Failed to write config.\n");
    return DRIVER_ERROR;
  }

  // Calibration calculation for 32V, 3.2A range with 0.1Ω shunt
  // With PGA = /8, max shunt voltage = ±320mV
  // Max current = 320mV / 100mΩ = 3.2A
  ina_dev->shunt_resistance = 0.1f;
  
  // Choose Current_LSB for 3.2A max current
  // Current_LSB = Max_Current / 32768 = 3.2 / 32768 = 0.0000976A
  // Round to 0.0001A (100µA) for simplicity
  ina_dev->current_lsb = 0.0001f;  // 100µA per bit = 0.1mA per bit
  
  // Calibration = 0.04096 / (Current_LSB * R_shunt)
  // Calibration = 0.04096 / (0.0001 * 0.1) = 0.04096 / 0.00001 = 4096
  ina_dev->calibration_value = (uint16_t)(0.04096f / (ina_dev->current_lsb * ina_dev->shunt_resistance));
  
  // Power LSB = 20 * Current_LSB
  ina_dev->power_lsb = 20.0f * ina_dev->current_lsb;

  bsp_printf("INA219: Current LSB: %.6f A/bit (%.3f mA/bit)\n", 
             ina_dev->current_lsb, ina_dev->current_lsb * 1000.0f);
  bsp_printf("INA219: Power LSB: %.6f W/bit (%.3f mW/bit)\n", 
             ina_dev->power_lsb, ina_dev->power_lsb * 1000.0f);
  bsp_printf("INA219: Writing calibration value: %d\n", ina_dev->calibration_value);
  
  if (ina219_write_register(ina_dev, INA219_REG_CALIBRATION, ina_dev->calibration_value) != 0) {
    bsp_printf("INA219: Failed to write calibration.\n");
    return DRIVER_ERROR;
  }

  chThdSleepMilliseconds(20);

  // Verify by reading back config
  uint16_t read_config;
  if(ina219_read_register(ina_dev, INA219_REG_CONFIG, &read_config) == 0){
      bsp_printf("INA219: Read back config: 0x%04X\n", read_config);
  } else {
      bsp_printf("INA219: Failed to read back config.\n");
      return DRIVER_ERROR;
  }

  // Read back calibration to verify
  uint16_t read_cal;
  if(ina219_read_register(ina_dev, INA219_REG_CALIBRATION, &read_cal) == 0){
      bsp_printf("INA219: Read back calibration: %d\n", read_cal);
  }

  bsp_printf("INA219: Initialization complete.\n");

  // Test readings
  uint16_t bus, shunt, current, power;
  for (int i = 0; i < 5; i++) {
    chThdSleepMilliseconds(40);
    
    ina219_read_register(ina_dev, INA219_REG_BUSVOLTAGE, &bus);
    ina219_read_register(ina_dev, INA219_REG_SHUNTVOLTAGE, &shunt);
    ina219_read_register(ina_dev, INA219_REG_CURRENT, &current);
    ina219_read_register(ina_dev, INA219_REG_POWER, &power);
    
    bsp_printf("BUS=0x%04X (CNVR=%d OVF=%d %.3fV) SHUNT=0x%04X CURRENT=0x%04X POWER=0x%04X\n",
               bus, (bus >> 1) & 1, bus & 1, ((bus >> 3) * 4) / 1000.0f,
               shunt, current, power);
  }

  return DRIVER_OK;
}
static int ina219_remove(device_t *dev) {
  (void)dev;
  return DRIVER_OK;
}

static int ina219_ioctl(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;
  (void)cmd;
  (void)arg;
  return DRIVER_NOT_FOUND;
}

static int ina219_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings) {
  if (dev == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > ina219_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM;
  }

  ina219_t *ina_dev_data = (ina219_t *)dev->priv;
  float shunt_v, bus_v, current_ma, power_mw;

  if (!ina219_read_all_helper(ina_dev_data, &shunt_v, &bus_v, &current_ma, &power_mw)) {
    return DRIVER_ERROR;
  }

  // Populate readings array
  for (uint32_t i = 0; i < num_readings; ++i) {
    const driver_reading_channel_t *channel = &ina219_readings_directory.channels[i];
    readings[i].type                        = channel->type;

    switch (channel->channel_type) {
      case READING_CHANNEL_TYPE_VOLTAGE:
        if (strcmp(channel->name, "shunt_voltage") == 0) {
          readings[i].value.float_val = shunt_v;
        } else if (strcmp(channel->name, "bus_voltage") == 0) {
          readings[i].value.float_val = bus_v;
        }
        break;
      case READING_CHANNEL_TYPE_CURRENT:
        readings[i].value.float_val = current_ma;
        break;
      case READING_CHANNEL_TYPE_POWER:
        readings[i].value.float_val = power_mw;
        break;
      default:
        // Should not happen
        break;
    }
  }

  return DRIVER_OK;
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
  //*power_mw = raw_value * (dev_data->power_lsb * 1000.0f);
  *power_mw = raw_value * dev_data->current_lsb * 20.0f * 1000.0f;

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
