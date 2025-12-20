/**
 * @file driver_registry.c
 * @brief Device registry and initialization implementation
 */
#include <string.h>

#include "drivers/driver_registry.h"
#include "drivers/bme280.h"
#include "drivers/ina219.h"
#include "drivers/ina3221.h"
#include "drivers/bh1750.h"
#include "drivers/aht2x.h"
#include "drivers/servo.h"

// External declarations for board devices (defined in bsp.c)
extern device_t board_devices[];
extern const size_t num_board_devices;

/**
 * @brief Array of all available drivers in the system
 *
 * This array contains pointers to all driver instances that are
 * compiled into the firmware. New drivers should be added here.
 */
const driver_t *drivers[] = {
  &bme280_driver,
  &ina219_driver,
  &ina3221_driver,
  &bh1750_driver,
  &aht2x_driver,
  //&servo_driver,
};

/** @brief Number of drivers in the system */
const size_t num_drivers = sizeof(drivers) / sizeof(drivers[0]);

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
void init_devices(void) {
  for (size_t i = 0; i < num_board_devices; i++) {
    device_t *dev = &board_devices[i];

    // Find matching driver for this device
    for (size_t j = 0; j < num_drivers; j++) {
      if (strcmp(dev->name, drivers[j]->name) == 0) {
        dev->driver = drivers[j];

        // Initialize the device if driver provides init function
        if (dev->driver->init) {
          dev->driver->init(dev);
        }
        break;
      }
    }
  }
}

/**
 * @brief Finds a device by name in the board_devices array.
 *
 * @param name The name of the device to find.
 * @return A pointer to the device_t structure if found, or NULL if not found.
 */
device_t *find_device(const char *name) {
  for (size_t i = 0; i < num_board_devices; i++) {
    if (strcmp(board_devices[i].name, name) == 0) {
      return &board_devices[i];
    }
  }
  return NULL;
}

/**
 * @brief Gets the list of all board devices.
 *
 * @param count Pointer to a size_t to store the number of devices.
 * @return A pointer to the array of device_t structures.
 */
device_t *get_board_devices(size_t *count) {
  *count = num_board_devices;
  return board_devices;
}
