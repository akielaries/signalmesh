#include "drivers/driver_registry.h"
#include "drivers/bme280.h"
#include "drivers/ina219.h"
#include "drivers/servo.h"
#include <string.h>

// Forward declaration of board-specific devices
extern device_t board_devices[];
extern const size_t num_board_devices;

// Array of all registered drivers in the system
const driver_t *drivers[] = {
    &bme280_driver,
    //&ina219_driver,
    //&servo_driver,
};
const size_t num_drivers = sizeof(drivers) / sizeof(drivers[0]);

void init_devices(void) {
    for (size_t i = 0; i < num_board_devices; i++) {
        device_t *dev = &board_devices[i];
        for (size_t j = 0; j < num_drivers; j++) {
            if (strcmp(dev->name, drivers[j]->name) == 0) {
                dev->driver = drivers[j];
                if (dev->driver->init) {
                    dev->driver->init(dev);
                }
                break;
            }
        }
    }
}
