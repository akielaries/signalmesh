#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/gm009605.h"

int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting OLED BIST (GM009605) ---\r\n");

  device_t *oled = find_device("gm009605");
  if (oled == NULL || oled->driver == NULL) {
    bsp_printf("OLED device or driver not found!\n");
    return -1;
  }

  if (oled->driver->clear == NULL || oled->driver->write == NULL) {
    bsp_printf("OLED driver does not support clear/write operations.\n");
    return -1;
  }

  bsp_printf("Clearing OLED display\n");
  oled->driver->clear(oled);

  bsp_printf("Writing to OLED display\n");
  const char *msg = "Hello, World!";
  oled->driver->write(oled, 0, msg, strlen(msg));

  bsp_printf("OLED BIST Finished.\n");

  return 0;
}

