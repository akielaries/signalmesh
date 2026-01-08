#include <stdint.h>
#include "hal.h"
#include "ch.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"

#include "common/utils.h"


int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting 74HC138 BIST ---\r\n");

  palSetPadMode(GPIOD, 0, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOD, 1, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOF, 9, PAL_MODE_OUTPUT_PUSHPULL);

  palSetPadMode(GPIOF, 7, PAL_MODE_ALTERNATE(0) | PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOG, 0, PAL_MODE_ALTERNATE(0) | PAL_MODE_OUTPUT_PUSHPULL);

  palWritePad(GPIOG, 0, 1);
  palWritePad(GPIOF, 7, 1);

  uint8_t i = 0;
  while (1) {
    bsp_printf("sel: %d\r\n", i);
    palWritePad(GPIOD, 0, (i >> 0) & 1);
    palWritePad(GPIOD, 1, (i >> 1) & 1);
    palWritePad(GPIOF, 9, (i >> 2) & 1);

    chThdSleepMilliseconds(500);
    i = (i + 1) % 8;
  }

  bsp_printf("\n--- End 74HC138 BIST ---\r\n");


  return 0;
}
