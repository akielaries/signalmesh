#include <math.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/driver_api.h"

// Forward declaration of board-specific devices
extern device_t board_devices[];
extern const size_t num_board_devices;


int main(void) {
  bsp_init();

  bsp_printf("--- Starting BME280 Test ---\r\n");
  bsp_printf("Testing BME280 sensor.\r\n");
  bsp_printf("Press button to stop.\r\n\r\n");

  // Find the bme280 device
  device_t *bme280_dev = NULL;
  for (size_t i = 0; i < num_board_devices; i++) {
    if (strcmp(board_devices[i].name, "bme280") == 0) {
      bme280_dev = &board_devices[i];
      break;
    }
  }

  if (bme280_dev == NULL) {
    bsp_printf("BME280 device not found!\n");
    while (1)
      ;
  }

  if (bme280_dev->driver->probe) {
    if (bme280_dev->driver->probe(bme280_dev) != DRIVER_OK) {
      bsp_printf("BME280 probe failed!\n");
      while (1)
        ;
    }
  }


  bsp_printf("finished probe\n");

  while (true) {
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("BME280 test stopped.\n");
      break;
    }

    if (bme280_dev->driver->poll) {
      bme280_dev->driver->poll(bme280_dev);
    }

    chThdSleepMilliseconds(1000);
  }

  return 0;
}
