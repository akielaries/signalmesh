#pragma once

#include "hal.h"

// Define GPIO pins for the debug UART
#define BSP_DEBUG_UART_TX_PORT     GPIOC
#define BSP_DEBUG_UART_TX_PIN      12
#define BSP_DEBUG_UART_RX_PORT     GPIOD
#define BSP_DEBUG_UART_RX_PIN      2
#define BSP_DEBUG_UART_AF          8 // Alternate Function for UART

// Extern declarations for the SerialConfig and SerialDriver instance
extern const SerialConfig bsp_debug_uart_config;
extern SerialDriver * const bsp_debug_uart_driver;
extern BaseSequentialStream *bsp_debug_stream;
