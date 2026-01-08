#include <stdint.h>
#include "hal.h"
#include "ch.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/spi.h"

#include "common/utils.h"


int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting SPI Mux select BIST ---\r\n");

  uint8_t device;
  while (1) {
    bsp_printf("device: %d\r\n", device);

    spi_cs_mux_select(device);


    chThdSleepMilliseconds(500);
    device = (device + 1) % 8;
  }

  bsp_printf("\n--- End 74HC138 BIST ---\r\n");


  return 0;
}
