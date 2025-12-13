/**
 * @file driver_registry.c
 * @brief Device registry and initialization implementation
 */
#include <string.h>

#include "drivers/driver_registry.h"
#include "drivers/bme280.h"
#include "drivers/ina219.h"
#include "drivers/servo.h"

/**
 * @brief Board-specific device array
 *
 * This must be defined in board-specific code. Each board defines
 * the devices that are present on that particular hardware configuration.
 */
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
  //&ina219_driver,
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
