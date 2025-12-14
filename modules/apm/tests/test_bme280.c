#include <math.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/driver_api.h"
#include "drivers/driver_registry.h" // For find_device()
#include "drivers/driver_readings.h" // For driver_reading_t

int main(void) {
  bsp_init();

  bsp_printf("--- Starting BME280 Test ---\r\n");
  bsp_printf("Testing BME280 sensor.\r\n");
  bsp_printf("Press button to stop.\r\n\r\n");

  int ret;
  // find the bme280 device
  device_t *bme280_dev = find_device("bme280");
  if (bme280_dev == NULL) {
    bsp_printf("BME280 device not found!\n");
    return -1;
  }

  bsp_printf("BME280 device found and initialized.\n");

  driver_reading_t bme280_readings[3]; // Temp, Pressure, Humidity
  uint32_t num_readings_to_get = 3;

  while (true) {
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("BME280 test stopped.\n");
      break;
    }

    ret = bme280_dev->driver->poll(bme280_dev,
                                   num_readings_to_get,
                                   bme280_readings);
    if (ret != DRIVER_OK) {
      bsp_printf("poll failed: 0x%X\n", ret);
    } else {
      bsp_printf("BME280 Readings:\n");
      for (uint32_t i = 0; i < num_readings_to_get; ++i) {
        const driver_reading_channel_t *channel =
          &bme280_dev->driver->readings_directory->channels[i];
        if (bme280_readings[i].type == READING_VALUE_TYPE_FLOAT) {
          if (strcmp(channel->name, "temperature") == 0) {
            float celsius    = bme280_readings[i].value.float_val;
            float fahrenheit = celsius * 1.8f + 32.0f;
            bsp_printf("  temperature: %.2f F\n", fahrenheit);
          } else if (strcmp(channel->name, "pressure") == 0) {
            float pascals = bme280_readings[i].value.float_val;
            float psi     = pascals * 0.0001450377f;
            float inhg    = pascals * 0.0002953f;
            bsp_printf("  pressure: %.2f PSI\n", psi);
            bsp_printf("  pressure: %.2f inHg\n", inhg);
          } else {
            bsp_printf("  %s: %.2f %s\n",
                       channel->name,
                       bme280_readings[i].value.float_val,
                       channel->unit);
          }
        } else {
          bsp_printf("  %s: Unknown type\n", channel->name);
        }
      }
    }


    chThdSleepMilliseconds(1000);
  }

  return 0;
}
