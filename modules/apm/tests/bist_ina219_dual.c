#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/driver_readings.h"

#define MAX_READINGS 8 // Should be large enough for any single driver

int main(void) {
  bsp_init();
  bsp_printf("--- Starting Dual INA219 Test ---\r\n");

  device_t *ina219_main_dev = find_device("ina219_main");
  device_t *ina219_aux_dev = find_device("ina219_aux");

  driver_reading_t readings[MAX_READINGS];
  int ret;

  while (true) {
    if (ina219_main_dev->driver && ina219_main_dev->driver->poll) {
      uint32_t num_readings = ina219_main_dev->driver->readings_directory->num_readings;
      ret = ina219_main_dev->driver->poll(ina219_main_dev, num_readings, readings);

      if (ret != DRIVER_OK) {
        bsp_printf("poll failed for %s: error %d\n", ina219_main_dev->name, ret);
      } else {
        bsp_printf("%s:\n", ina219_main_dev->name);
        for (uint32_t i = 0; i < num_readings; i++) {
          const driver_reading_channel_t *channel = &ina219_main_dev->driver->readings_directory->channels[i];
          bsp_printf("  %s: %.2f %s\n", channel->name, readings[i].value.float_val, channel->unit);
        }
      }
    }

    if (ina219_aux_dev->driver && ina219_aux_dev->driver->poll) {
      uint32_t num_readings = ina219_aux_dev->driver->readings_directory->num_readings;
      ret = ina219_aux_dev->driver->poll(ina219_aux_dev, num_readings, readings);

      if (ret != DRIVER_OK) {
        bsp_printf("poll failed for %s: error %d\n", ina219_aux_dev->name, ret);
      } else {
        bsp_printf("%s:\n", ina219_aux_dev->name);
        for (uint32_t i = 0; i < num_readings; i++) {
          const driver_reading_channel_t *channel = &ina219_aux_dev->driver->readings_directory->channels[i];
          bsp_printf("  %s: %.2f %s\n", channel->name, readings[i].value.float_val, channel->unit);
        }
      }
    }

    bsp_printf("----------------------------------------\n");
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
