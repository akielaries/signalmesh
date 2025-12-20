#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/bh1750.h"
#include "ch.h"
#include "hal.h"
#include "drivers/driver_readings.h"
#include "drivers/i2c.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// forward declarations
static int bh1750_init(device_t *dev);
static int bh1750_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings);

// helper function to send a command to the BH1750
static int bh1750_send_command(bh1750_t *dev, uint8_t cmd) {
    msg_t ret = i2c_master_transmit(dev->bus.i2c, dev->bus.addr, &cmd, 1, NULL, 0);
    if (ret != MSG_OK) {
        bsp_printf("BH1750: Failed to send command 0x%02X\n", cmd);
        i2c_handle_error(dev->bus.i2c, "bh1750_send_command");
        return DRIVER_ERROR;
    }
    return DRIVER_OK;
}

// BH1750 defines 1 reading (illuminance)
static const driver_reading_channel_t bh1750_reading_channels[] = {
    {.name = "illuminance", .unit = "lux", .type = READING_VALUE_TYPE_FLOAT},
};

static const driver_readings_directory_t bh1750_readings_directory = {
    .num_readings = ARRAY_SIZE(bh1750_reading_channels),
    .channels = bh1750_reading_channels,
};

const driver_t bh1750_driver __attribute__((used)) = {
    .name = "bh1750",
    .init = bh1750_init,
    .poll = (int (*)(device_id_t, uint32_t, driver_reading_t *))bh1750_poll,
    .readings_directory = &bh1750_readings_directory,
};

static int bh1750_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("BH1750 init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  bh1750_t *bh_dev = (bh1750_t *)dev->priv;

  bsp_printf("BH1750: Initializing...\n");

  bh_dev->bus.i2c = (I2CDriver *)dev->bus;
  bh_dev->bus.addr = BH1750_DEFAULT_ADDRESS;

  // power on the device
  if (bh1750_send_command(bh_dev, BH1750_CMD_POWER_ON) != 0) {
      return DRIVER_ERROR;
  }
  chThdSleepMilliseconds(5);

  // set continuous high-resolution mode
  if (bh1750_send_command(bh_dev, BH1750_CMD_CONT_H_RES_MODE) != 0) {
      return DRIVER_ERROR;
  }
  chThdSleepMilliseconds(120); // wait for first measurement

  bsp_printf("BH1750: Initialization complete.\n");
  return DRIVER_OK;
}

static int bh1750_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings) {
  if (dev == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  bh1750_t *bh_dev = (bh1750_t *)dev->priv;
  uint8_t buf[2];

  msg_t ret = i2c_master_receive(bh_dev->bus.i2c, bh_dev->bus.addr, buf, 2);
  if (ret != MSG_OK) {
    bsp_printf("BH1750: Failed to read data.\n");
    i2c_handle_error(bh_dev->bus.i2c, "bh1750_poll");
    return DRIVER_ERROR;
  }

  uint16_t raw_value = (uint16_t)buf[0] << 8 | (uint16_t)buf[1];
  float lux = (float)raw_value / 1.2f;

  readings[0].value.float_val = lux;
  readings[0].type = READING_VALUE_TYPE_FLOAT;

  return DRIVER_OK;
}
