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

    ret = bme280_dev->driver->poll(bme280_dev, num_readings_to_get, bme280_readings);
    if (ret != DRIVER_OK) {
      bsp_printf("poll failed: 0x%X\n", ret);
    } else {
      bsp_printf("BME280 Readings:\n");
      for (uint32_t i = 0; i < num_readings_to_get; ++i) {
        if (bme280_readings[i].type == READING_VALUE_TYPE_UINT32) {
          const char* name = bme280_dev->driver->readings_directory->channels[i].name;
          uint32_t raw_val = bme280_readings[i].value.u32_val;
          float value = 0.0f;
          const char* unit = "";

          if (strcmp(name, "temperature") == 0) {
            value = raw_val / 100.0f;
            unit = "C";
          } else if (strcmp(name, "pressure") == 0) {
            value = raw_val / 256.0f;
            unit = "Pa";
          } else if (strcmp(name, "humidity") == 0) {
            value = raw_val / 1024.0f;
            unit = "%RH";
          }

          bsp_printf("  %s: %.2f %s\n", name, value, unit);

        } else {
          bsp_printf("  %s: Unknown type\n",
                     bme280_dev->driver->readings_directory->channels[i].name);
        }
      }
    }


    chThdSleepMilliseconds(1000);
  }

  return 0;
}
