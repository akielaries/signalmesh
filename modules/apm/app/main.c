/**
 * @brief Audio Peripheral Module (APM) main application entry point
 */

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"



int main(void) {
  bsp_init(); // Call the unified BSP initialization function

  bsp_printf("Audio Peripheral Module (APM) Application v1.0.0\n");

  while (true) {
    chThdSleepMilliseconds(500);
  }
}
