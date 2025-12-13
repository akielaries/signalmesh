#include "drivers/servo.h"
#include "bsp/configs/bsp_pwm_config.h"
#include "hal.h" // For palSetPadMode, pwmStart etc.

// Servo-specific pulse widths
#define SERVO_MIN_US 600
#define SERVO_MAX_US 2400

static int servo_init_fn(device_t *dev) {
  (void)dev;
  // Configure GPIO for PWM output
  palSetPadMode(BSP_SERVO_GPIO_PORT,
                BSP_SERVO_GPIO_PIN,
                PAL_MODE_ALTERNATE(BSP_SERVO_GPIO_ALT_MODE));

  // Start the PWM driver
  pwmStart(bsp_servo_pwm_driver, &bsp_servo_pwm_config);
  return DRIVER_OK;
}

static int servo_probe_fn(device_t *dev) {
  (void)dev;
  // Add servo specific probing logic here
  return DRIVER_OK;
}

static int servo_remove_fn(device_t *dev) {
  (void)dev;
  // Add servo specific removal logic here
  pwmStop(bsp_servo_pwm_driver);
  return DRIVER_OK;
}

static int servo_ioctl_fn(device_t *dev, uint32_t cmd, void *arg) {
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

static int servo_poll_fn(device_t *dev) {
  (void)dev;
  // Servo does not need polling
  return DRIVER_OK;
}

const driver_t servo_driver __attribute__((used)) = {
  .name   = "servo",
  .init   = servo_init_fn,
  .probe  = servo_probe_fn,
  .remove = servo_remove_fn,
  .ioctl  = servo_ioctl_fn,
  .poll   = servo_poll_fn,
};
