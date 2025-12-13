#pragma once

#include <stdint.h>
#include "drivers/driver_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// IOCTL commands for servo
#define SERVO_SET_POS_CMD 1
#define SERVO_SET_ANGLE_CMD 2
#define SERVO_STOP_CMD 3

extern const driver_t servo_driver;

#ifdef __cplusplus
}
#endif
