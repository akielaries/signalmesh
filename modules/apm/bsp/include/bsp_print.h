#pragma once
/**
 * @brief bsp_print.h
 * @description debug printing for the STM32H755
 */

#include "chprintf.h"
#include "hal.h"

/** @brief debug port GPIOs */
#define APM_BSP_DBG_TX GPIOC_PIN12
#define APM_BSP_DBG_RX GPIOD_PIN2


/* Extract just the filename from full path */
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                      strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

/* Basic printf - no formatting */
#define bsp_printf(fmt, ...) \
    chprintf(apm_debug_stream, fmt, ##__VA_ARGS__)

/* Error printf with file and line */
#define bsp_printf_err(fmt, ...) \
    chprintf(apm_debug_stream, "[ERR][%s:%d] " fmt, __FILENAME__, __LINE__, ##__VA_ARGS__)

/* Warning printf with file and line */
#define bsp_printf_warn(fmt, ...) \
    chprintf(apm_debug_stream, "[WARN][%s:%d] " fmt, __FILENAME__, __LINE__, ##__VA_ARGS__)

/* Info printf with file and line */
#define bsp_printf_info(fmt, ...) \
    chprintf(apm_debug_stream, "[INFO][%s:%d] " fmt, __FILENAME__, __LINE__, ##__VA_ARGS__)

