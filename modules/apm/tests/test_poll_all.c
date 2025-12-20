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

  bsp_printf("--- Starting driver test ---\r\n");
  bsp_printf("Press button to stop...\r\n\r\n");

  size_t num_devices;
  device_t *devices = get_board_devices(&num_devices);

  if (devices == NULL) {
    bsp_printf("Failed to get board devices!\n");
    return -1;
  }

  bsp_printf("Found %d devices.\n", num_devices);

  driver_reading_t readings[4];
  uint32_t num_readings_to_get = 4;
  int ret;

  while (true) {
    for (size_t i = 0; i < num_devices; i++) {
      device_t *dev = &devices[i];

      if (dev->driver && dev->driver->poll) {
        ret = dev->driver->poll(dev, num_readings_to_get, readings);

        if (ret != DRIVER_OK) {
          bsp_printf("poll failed for %s: 0x%X\n", dev->name, ret);
        } else {
          bsp_printf("\r\n%s Readings:\n", dev->name);
          for (uint32_t j = 0; j < dev->driver->readings_directory->num_readings; j++) {
            const driver_reading_channel_t *channel =
              &dev->driver->readings_directory->channels[j];

            if (channel && readings[j].type == channel->type) {
                bsp_printf("  %s: %.2f %s\n",
                           channel->name,
                           readings[j].value.float_val,
                           channel->unit);
            } else {
              bsp_printf("  Unknown channel type\n");
            }
          }
        }
      }
    }


    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("Polling stopped.\n");
      break;
    }
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
