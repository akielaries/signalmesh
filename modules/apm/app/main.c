#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h" // Include the new bsp_init function
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h" // For bsp_debug_stream

int main(void) {
  bsp_init(); // Call the unified BSP initialization function

  bsp_printf("Hello from APM main!\n"); // Use chprintf with bsp_debug_stream

  while (true) {
    // Placeholder for the main application loop
    chThdSleepMilliseconds(500);
  }
}
