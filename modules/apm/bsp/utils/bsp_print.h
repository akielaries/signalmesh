#pragma once

#include <stdarg.h>
#include <stdint.h> // For uint8_t in print_getchar
#include "chprintf.h" // For chprintf
#include "bsp/configs/bsp_uart_config.h" // For bsp_debug_stream

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the debug printing utility.
 *        This configures the UART GPIOs and starts the serial driver.
 */
void bsp_print_init(void);

/**
 * @brief Reads a single character from the debug input stream.
 * @return The character read.
 */
char bsp_print_getchar(void);

#ifdef __cplusplus
}
#endif

// Macro for debug printing, directly wraps chprintf. Newlines must be explicitly added by the caller.
#define debug_printf(fmt, ...) \
    chprintf(bsp_debug_stream, fmt, ##__VA_ARGS__)
