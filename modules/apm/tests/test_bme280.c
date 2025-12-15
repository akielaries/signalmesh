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

  driver_reading_t bme280_readings[4];
  uint32_t num_readings_to_get = 4;

  while (true) {
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

        if (channel && bme280_readings[i].type == channel->type) {
            bsp_printf("  %s: %.2f %s\n",
                       channel->name,
                       bme280_readings[i].value.float_val,
                       channel->unit);
        } else {
          bsp_printf("  Unknown channel type\n");
        }
      }
    }

    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("BME280 test stopped.\n");
      break;
    }
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
