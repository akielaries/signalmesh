#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/aht2x.h"
#include "ch.h"
#include "hal.h"
#include "drivers/driver_readings.h"
#include "drivers/i2c.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int aht2x_init(device_t *dev);
static int aht2x_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings);

// AHT2x defines 2 readings (temperature and humidity)
static const driver_reading_channel_t aht2x_reading_channels[] = {
  {
    .channel_type = READING_CHANNEL_TYPE_TEMPERATURE_F,
    .name         = "temperature",
    .unit         = "F",
    .type         = READING_VALUE_TYPE_FLOAT,
  },
  {
    .channel_type = READING_CHANNEL_TYPE_HUMIDITY,
    .name = "humidity",
    .unit = "%RH",
    .type = READING_VALUE_TYPE_FLOAT,
  },
};

static const driver_readings_directory_t aht2x_readings_directory = {
  .num_readings = ARRAY_SIZE(aht2x_reading_channels),
  .channels     = aht2x_reading_channels,
};

const driver_t aht2x_driver __attribute__((used)) = {
  .name               = "aht2x",
  .init               = aht2x_init,
  .poll               = (int (*)(device_id_t, uint32_t, driver_reading_t *))aht2x_poll,
  .readings_directory = &aht2x_readings_directory,
};

static int aht2x_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("AHT2x init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  aht2x_t *aht_dev = (aht2x_t *)dev->priv;

  bsp_printf("AHT2x: Initializing...\n");

  aht_dev->bus.i2c    = (I2CDriver *)dev->bus;
  aht_dev->bus.addr   = AHT2X_DEFAULT_ADDRESS;
  aht_dev->calibrated = false;

  chThdSleepMilliseconds(40); // wait for power on

  // check status
  uint8_t status;
  msg_t ret = i2c_master_receive(aht_dev->bus.i2c, aht_dev->bus.addr, &status, 1);
  if (ret != MSG_OK) {
    bsp_printf("AHT2x: Failed to read status on init.\n");
    i2c_handle_error(aht_dev->bus.i2c, "aht2x_init_status_read");
    return DRIVER_ERROR;
  }

  if (status & 0x08) { // check calibration bit
    aht_dev->calibrated = true;
    bsp_printf("AHT2x: Already calibrated.\n");
  } else {
    bsp_printf("AHT2x: Not calibrated. Initializing...\n");
    uint8_t init_cmd[] = {AHT2X_CMD_INITIALIZE, 0x08, 0x00};
    ret =
      i2c_master_transmit(aht_dev->bus.i2c, aht_dev->bus.addr, init_cmd, sizeof(init_cmd), NULL, 0);
    if (ret != MSG_OK) {
      bsp_printf("AHT2x: Failed to send init command.\n");
      i2c_handle_error(aht_dev->bus.i2c, "aht2x_init_cmd");
      return DRIVER_ERROR;
    }
    chThdSleepMilliseconds(10);
    aht_dev->calibrated = true;
  }

  bsp_printf("AHT2x: Initialization complete.\n");
  return DRIVER_OK;
}

static int aht2x_poll(device_t *dev, uint32_t num_readings, driver_reading_t *readings) {
  if (dev == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  aht2x_t *aht_dev = (aht2x_t *)dev->priv;

  // trigger measurement
  uint8_t trigger_cmd[] = {AHT2X_CMD_TRIGGER_MEASUREMENT, 0x33, 0x00};
  msg_t ret             = i2c_master_transmit(aht_dev->bus.i2c,
                                  aht_dev->bus.addr,
                                  trigger_cmd,
                                  sizeof(trigger_cmd),
                                  NULL,
                                  0);
  if (ret != MSG_OK) {
    bsp_printf("AHT2x: Failed to trigger measurement.\n");
    i2c_handle_error(aht_dev->bus.i2c, "aht2x_poll_trigger");
    return DRIVER_ERROR;
  }

  // wait for measurement to complete
  chThdSleepMilliseconds(80);

  // read data
  uint8_t buf[7];
  ret = i2c_master_receive(aht_dev->bus.i2c, aht_dev->bus.addr, buf, 7);
  if (ret != MSG_OK) {
    bsp_printf("AHT2x: Failed to read data.\n");
    i2c_handle_error(aht_dev->bus.i2c, "aht2x_poll_read");
    return DRIVER_ERROR;
  }

  // check busy status
  if (buf[0] & 0x80) {
    bsp_printf("AHT2x: Sensor is busy.\n");
    return DRIVER_ERROR; // or maybe retry? For now, error.
  }

  // convert data
  uint32_t raw_humidity = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
  float humidity        = ((float)raw_humidity / 1048576.0f) * 100.0f; // 2^20 = 1048576

  uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
  float temperature = (((float)raw_temp / 1048576.0f) * 200.0f) - 50.0f;
  temperature       = temperature * 1.8f + 32.0f;

  readings[0].value.float_val = temperature;
  readings[0].type            = READING_VALUE_TYPE_FLOAT;
  readings[1].value.float_val = humidity;
  readings[1].type            = READING_VALUE_TYPE_FLOAT;

  return DRIVER_OK;
}
