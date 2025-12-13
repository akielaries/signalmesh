#include "drivers/servo.h"
#include "bsp/configs/bsp_pwm_config.h"
#include "hal.h"                     // For palSetPadMode, pwmStart etc.
#include <string.h>                  // For strcmp
#include "drivers/driver_readings.h" // Include the new readings definitions

// Macro to calculate the size of an array
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Servo-specific pulse widths
#define SERVO_MIN_US 600
#define SERVO_MAX_US 2400

// Servo specific reading channels (commanded position in microseconds)
static const driver_reading_channel_t servo_reading_channels[] = {
  {.name = "commanded_position_us", .type = READING_VALUE_TYPE_UINT32},
};

// Servo readings directory
static const driver_readings_directory_t servo_readings_directory = {
  .num_readings = ARRAY_SIZE(servo_reading_channels),
  .channels     = servo_reading_channels,
};

static int servo_init(device_t *dev) {
  (void)dev;
  // Configure GPIO for PWM output
  palSetPadMode(BSP_SERVO_GPIO_PORT,
                BSP_SERVO_GPIO_PIN,
                PAL_MODE_ALTERNATE(BSP_SERVO_GPIO_ALT_MODE));

  // Start the PWM driver
  pwmStart(bsp_servo_pwm_driver, &bsp_servo_pwm_config);
  return DRIVER_OK;
}

static int servo_remove(device_t *dev) {
  (void)dev;
  // Add servo specific removal logic here
  pwmStop(bsp_servo_pwm_driver);
  return DRIVER_OK;
}

static int servo_ioctl(device_t *dev, uint32_t cmd, void *arg) {
  (void)dev;

  switch (cmd) {
    case SERVO_SET_POS_CMD: {
      uint16_t us = *(uint16_t *)arg;
      if (us < SERVO_MIN_US)
        us = SERVO_MIN_US;
      if (us > SERVO_MAX_US)
        us = SERVO_MAX_US;
      pwmEnableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL, us);
      break;
    }
    case SERVO_SET_ANGLE_CMD: {
      float degrees = *(float *)arg;
      if (degrees < 0)
        degrees = 0;
      if (degrees > 180)
        degrees = 180;
      uint16_t us = SERVO_MIN_US + (uint16_t)((degrees / 180.0f) *
                                              (SERVO_MAX_US - SERVO_MIN_US));
      pwmEnableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL, us);
      break;
    }
    case SERVO_STOP_CMD: {
      pwmDisableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL);
      break;
    }
    default:
      return DRIVER_INVALID_PARAM;
  }

  return DRIVER_OK;
}

static int servo_poll(device_id_t device_id,
                      uint32_t num_readings,
                      driver_reading_t *readings) {
  // (void)device_id; // Unused for now
  if (device_id == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > servo_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM; // Caller asked for more readings than
                                 // available
  }

  // Currently, the servo driver does not maintain its state in dev->priv
  // If it did, we would populate 'readings' here.
  // For now, return DRIVER_OK if num_readings matches, but don't fill.
  // Or, if no meaningful data is available, return DRIVER_NOT_FOUND.

  // Assuming for now that we want to report the "commanded_position_us" if
  // available This would require storing the last commanded 'us' value in a
  // servo_t struct and assigning it to dev->priv. Since that's not present,
  // we'll return an error if a reading is actually requested.

  if (num_readings == 1 && strcmp(servo_readings_directory.channels[0].name,
                                  "commanded_position_us") == 0) {
    // Here, you would retrieve the actual commanded position from
    // device_id->priv and assign it to readings[0].value.u32_val For now,
    // returning DRIVER_NOT_FOUND as we don't store it.
    return DRIVER_NOT_FOUND;
  }

  return DRIVER_NOT_FOUND; // Default if no specific reading could be provided
}

const driver_t servo_driver __attribute__((used)) = {
  .name               = "servo",
  .init               = servo_init,
  .remove             = servo_remove,
  .ioctl              = servo_ioctl,
  .poll               = servo_poll,
  .readings_directory = &servo_readings_directory,
};
