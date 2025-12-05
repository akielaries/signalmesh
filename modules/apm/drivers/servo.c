#include "drivers/servo.h"
#include "bsp/configs/bsp_pwm_config.h"
#include "hal.h" // For palSetPadMode, pwmStart etc.

// Servo-specific pulse widths
#define SERVO_MIN_US 600
#define SERVO_MAX_US 2400

void servo_init(void) {
  // Configure GPIO for PWM output
  palSetPadMode(BSP_SERVO_GPIO_PORT, BSP_SERVO_GPIO_PIN, PAL_MODE_ALTERNATE(BSP_SERVO_GPIO_ALT_MODE));
  
  // Start the PWM driver
  pwmStart(bsp_servo_pwm_driver, &bsp_servo_pwm_config);
}

void servo_set_pos(uint16_t us) {
  if (us < SERVO_MIN_US) us = SERVO_MIN_US;
  if (us > SERVO_MAX_US) us = SERVO_MAX_US;
  
  // Enable the channel with the specified pulse width
  pwmEnableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL, us);
}

void servo_set_angle(float degrees) {
  if (degrees < 0) degrees = 0;
  if (degrees > 180) degrees = 180;
  
  // Map degrees (0-180) to the servo's pulse width range
  uint16_t us = SERVO_MIN_US + (uint16_t)((degrees / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
  servo_set_pos(us);
}

void servo_stop(void) {
    pwmDisableChannel(bsp_servo_pwm_driver, BSP_SERVO_PWM_CHANNEL);
}
