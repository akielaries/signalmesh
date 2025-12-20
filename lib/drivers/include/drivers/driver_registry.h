/**
 * @file driver_registry.h
 * @brief Device registry and initialization functions
 *
 * This file provides functions for managing device registration and
 * initialization. It handles the automatic matching of board devices
 * to their respective drivers during system startup.
 */

#pragma once

#include "driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all devices defined in board configuration
 *
 * This function iterates through all devices defined in the board-specific
 * device array and matches them with their corresponding drivers based on name.
 * For each matched device, it calls the driver's init function.
 *
 * This should be called once during system startup after all drivers are
 * registered and board devices are defined.
 */
void init_devices(void);
device_t *find_device(const char *name);
device_t *get_board_devices(size_t *count);

#ifdef __cplusplus
}
#endif
