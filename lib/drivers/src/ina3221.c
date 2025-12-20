#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/ina3221.h"
#include "ch.h"
#include "hal.h"
#include "drivers/driver_readings.h"
#include "drivers/i2c.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


/******************************************************************************/
// Function prototypes
/******************************************************************************/

static int ina3221_init(device_t *dev);
static int ina3221_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings);

/******************************************************************************/
// Local types
/******************************************************************************/

static const driver_reading_channel_t ina3221_reading_channels[] = {
  // Channel 1
  {
    .name = "ch1_shunt_voltage",
    .unit = "mV",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch1_bus_voltage",
    .unit = "V",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch1_current",
    .unit = "mA",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch1_power",
    .unit = "mW",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  // Channel 2
  {
    .name = "ch2_shunt_voltage",
    .unit = "mV",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch2_bus_voltage",
    .unit = "V",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch2_current",
    .unit = "mA",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch2_power",
    .unit = "mW",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  // Channel 3
  {
    .name = "ch3_shunt_voltage",
    .unit = "mV",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch3_bus_voltage",
    .unit = "V",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch3_current",
    .unit = "mA",
    .type = READING_VALUE_TYPE_FLOAT,
  },
  {
    .name = "ch3_power",
    .unit = "mW",
    .type = READING_VALUE_TYPE_FLOAT,
  },
};

static const driver_readings_directory_t ina3221_readings_directory = {
  .num_readings = ARRAY_SIZE(ina3221_reading_channels),
  .channels     = ina3221_reading_channels,
};

const driver_t ina3221_driver __attribute__((used)) = {
  .name               = "ina3221",
  .init               = ina3221_init,
  .poll               = (int (*)(device_id_t, uint32_t, driver_reading_t *))ina3221_poll,
  .readings_directory = &ina3221_readings_directory,
};


/******************************************************************************/
// Local driver helper functions
/******************************************************************************/

static int ina3221_read_register(ina3221_t *dev, uint8_t reg, uint16_t *val) {
  uint8_t buf[2];
  int ret = i2c_bus_read_reg(&dev->bus, reg, buf, 2);
  if (ret != 0) {
    bsp_printf("INA3221 read_reg failed: reg=0x%02X, err=%d\n", reg, ret);
    return ret;
  }
  *val = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
  return 0;
}

static int ina3221_write_register(ina3221_t *dev, uint8_t reg, uint16_t val) {
  uint8_t buf[2];
  buf[0]  = (uint8_t)(val >> 8);
  buf[1]  = (uint8_t)(val & 0xFF);
  int ret = i2c_bus_write_reg(&dev->bus, reg, buf, 2);
  if (ret != 0) {
    bsp_printf("INA3221 write_reg failed: reg=0x%02X, err=%d\n", reg, ret);
  }
  return ret;
}

/******************************************************************************/
// Exported driver functions
/******************************************************************************/

static int ina3221_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("INA3221 init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  ina3221_t *ina_dev = (ina3221_t *)dev->priv;

  bsp_printf("INA3221: Initializing...\n");

  ina_dev->bus.i2c  = (I2CDriver *)dev->bus;
  ina_dev->bus.addr = INA3221_DEFAULT_ADDRESS;

  // Set shunt resistor values (assuming 0.1 ohm for all channels for now)
  ina_dev->shunt_resistors[0] = 0.1f;
  ina_dev->shunt_resistors[1] = 0.1f;
  ina_dev->shunt_resistors[2] = 0.1f;

  bsp_printf("INA3221: Resetting device...\n");
  if (ina3221_write_register(ina_dev, INA3221_REG_CONFIG, INA3221_CONFIG_RESET) != 0) {
    bsp_printf("INA3221: Failed to reset device.\n");
    return DRIVER_ERROR;
  }
  chThdSleepMilliseconds(5);

  uint16_t config_value = INA3221_CONFIG_CH1_EN | INA3221_CONFIG_CH2_EN | INA3221_CONFIG_CH3_EN |
                          INA3221_CONFIG_AVG_16 | INA3221_CONFIG_VBUS_CT_1100US |
                          INA3221_CONFIG_VSH_CT_1100US | INA3221_CONFIG_MODE_SHUNT_BUS_CONTINUOUS;

  bsp_printf("INA3221: Writing config value: 0x%04X\n", config_value);
  if (ina3221_write_register(ina_dev, INA3221_REG_CONFIG, config_value) != 0) {
    bsp_printf("INA3221: Failed to write config.\n");
    return DRIVER_ERROR;
  }

  bsp_printf("INA3221: Initialization complete.\n");
  return DRIVER_OK;
}

static int ina3221_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings) {
  if (dev == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  ina3221_t *ina_dev = (ina3221_t *)dev->priv;

  uint8_t shunt_reg_addrs[] = {INA3221_REG_CH1_SHUNTVOLTAGE,
                               INA3221_REG_CH2_SHUNTVOLTAGE,
                               INA3221_REG_CH3_SHUNTVOLTAGE};
  uint8_t bus_reg_addrs[]   = {INA3221_REG_CH1_BUSVOLTAGE,
                               INA3221_REG_CH2_BUSVOLTAGE,
                               INA3221_REG_CH3_BUSVOLTAGE};

  for (int ch = 0; ch < 3; ++ch) {
    uint16_t shunt_voltage_raw, bus_voltage_raw;

    if (ina3221_read_register(ina_dev, shunt_reg_addrs[ch], &shunt_voltage_raw) != 0)
      return DRIVER_ERROR;
    if (ina3221_read_register(ina_dev, bus_reg_addrs[ch], &bus_voltage_raw) != 0)
      return DRIVER_ERROR;

    // Values are signed, 12-bit right-aligned
    float shunt_voltage_mv = ((int16_t)shunt_voltage_raw >> 3) * INA3221_SHUNT_VOLTAGE_LSB;
    float bus_voltage_v    = ((int16_t)bus_voltage_raw >> 3) * INA3221_BUS_VOLTAGE_LSB;
    float current_ma       = (shunt_voltage_mv / 1000.0f) / ina_dev->shunt_resistors[ch] * 1000.0f;
    float power_mw         = bus_voltage_v * current_ma;

    readings[ch * 4 + 0].value.float_val = shunt_voltage_mv;
    readings[ch * 4 + 1].value.float_val = bus_voltage_v;
    readings[ch * 4 + 2].value.float_val = current_ma;
    readings[ch * 4 + 3].value.float_val = power_mw;

    readings[ch * 4 + 0].type = READING_VALUE_TYPE_FLOAT;
    readings[ch * 4 + 1].type = READING_VALUE_TYPE_FLOAT;
    readings[ch * 4 + 2].type = READING_VALUE_TYPE_FLOAT;
    readings[ch * 4 + 3].type = READING_VALUE_TYPE_FLOAT;
  }

  return DRIVER_OK;
}
