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
  {
    .channel_type = READING_CHANNEL_TYPE_POSITION_US,
    .name         = "position",
    .unit         = "us",
    .type         = READING_VALUE_TYPE_UINT32,
  },
};

// Servo readings directory
static const driver_readings_directory_t servo_readings_directory = {
  .num_readings = ARRAY_SIZE(servo_reading_channels),
  .channels     = servo_reading_channels,
};

//TODO: this is specific to what ever mgr servo i'm using really
servo_t gl_servo_dev = {0};

static int servo_init(device_t *dev) {
  if (dev == NULL) {
    return DRIVER_INVALID_PARAM;
  }

  servo_t *servo_dev = &gl_servo_dev;
  if (servo_dev == NULL) {
    return DRIVER_ERROR;
  }
  memset(servo_dev, 0, sizeof(servo_t));
  dev->priv = servo_dev;

  // Configure GPIO for PWM output
  palSetPadMode(BSP_SERVO_GPIO_PORT,
                BSP_SERVO_GPIO_PIN,
                PAL_MODE_ALTERNATE(BSP_SERVO_GPIO_ALT_MODE));

  // Start the PWM driver
  pwmStart(bsp_servo_pwm_driver, &bsp_servo_pwm_config);

  // Initialize with a default position (e.g., center)
  servo_dev->last_commanded_position_us = (SERVO_MIN_US + SERVO_MAX_US) / 2;
  pwmEnableChannel(bsp_servo_pwm_driver,
                   BSP_SERVO_PWM_CHANNEL,
                   servo_dev->last_commanded_position_us);

  return DRIVER_OK;
}

static int servo_remove(device_t *dev) {
  (void)dev;
  // Add servo specific removal logic here
  pwmStop(bsp_servo_pwm_driver);
  return DRIVER_OK;
}

static int servo_ioctl(device_t *dev, uint32_t cmd, void *arg) {
  servo_t *servo_dev = (servo_t *)dev->priv;

  switch (cmd) {
    case SERVO_SET_POS_CMD: {
      uint16_t us = *(uint16_t *)arg;
      if (us < SERVO_MIN_US)
        us = SERVO_MIN_US;
      if (us > SERVO_MAX_US)
        us = SERVO_MAX_US;
      pwmEnableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL, us);
      servo_dev->last_commanded_position_us = us;
      break;
    }
    case SERVO_SET_ANGLE_CMD: {
      float degrees = *(float *)arg;
      if (degrees < 0)
        degrees = 0;
      if (degrees > 180)
        degrees = 180;
      uint16_t us = SERVO_MIN_US + (uint16_t)((degrees / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
      pwmEnableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL, us);
      servo_dev->last_commanded_position_us = us;
      break;
    }
    case SERVO_STOP_CMD: {
      pwmDisableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL);
      servo_dev->last_commanded_position_us = 0; // Indicate stopped
      break;
    }
    default:
      return DRIVER_INVALID_PARAM;
  }

  return DRIVER_OK;
}

static int servo_poll(device_id_t device_id, uint32_t num_readings, driver_reading_t *readings) {
  if (device_id == NULL || readings == NULL || num_readings == 0) {
    return DRIVER_INVALID_PARAM;
  }

  if (num_readings > servo_readings_directory.num_readings) {
    return DRIVER_INVALID_PARAM; // Caller asked for more readings than
                                 // available
  }

  servo_t *servo_dev = (servo_t *)device_id->priv;

  // Populate readings array based on directory
  for (uint32_t i = 0; i < num_readings; ++i) {
    const driver_reading_channel_t *channel = &servo_readings_directory.channels[i];

    if (channel == NULL) { // Should not happen
      return DRIVER_ERROR;
    }

    readings[i].type = channel->type;

    switch (servo_readings_directory.channels[i].channel_type) {
      case READING_CHANNEL_TYPE_POSITION_US:
        readings[i].value.u32_val = servo_dev->last_commanded_position_us;
        break;
      default:
        return DRIVER_ERROR; // Should not happen with current setup
    }
  }

  return DRIVER_OK;
}

const driver_t servo_driver __attribute__((used)) = {
  .name               = "servo",
  .init               = servo_init,
  .remove             = servo_remove,
  .ioctl              = servo_ioctl,
  .poll               = servo_poll,
  .readings_directory = &servo_readings_directory,
};
