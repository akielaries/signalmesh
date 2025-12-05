#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h" // Include the new bsp_init function
#include "chprintf.h" // For chprintf
#include "bsp/configs/bsp_uart_config.h" // For bsp_debug_stream

int main(void) {
    bsp_init(); // Call the unified BSP initialization function

    chprintf(bsp_debug_stream, "Hello from APM main!\r\n"); // Use chprintf with bsp_debug_stream

    while (true) {
        // Placeholder for the main application loop
        chThdSleepMilliseconds(500);
    }
}
