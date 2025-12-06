#pragma once

#include <stdarg.h>
#include <stdint.h>
#include "chprintf.h"
#include "bsp/configs/bsp_uart_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the BSP IO utilities.
 *        This configures the UART GPIOs and starts the serial driver.
 */
void bsp_io_init(void);

/**
 * @brief Reads a single character from the debug input stream.
 * @return The character read.
 */
char bsp_io_getchar(void);

/**
 * @brief Simple printf wrapper that automatically adds \r before \n
 * @param fmt Format string
 * @param ... Variable arguments
 */
/**
 * @brief Simple printf wrapper that automatically adds \r for proper terminal
 * formatting
 * @param fmt Format string
 * @param ... Variable arguments
 */
void bsp_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
