#include "bsp/configs/bsp_pwm_config.h"

// PWM configuration for the servo motor
const PWMConfig bsp_servo_pwm_config = {
  .frequency  = 1000000, // 1MHz clock
  .period     = 20000,   // 20ms period (50Hz), standard for servos
  .callback   = NULL,
  .channels   = {
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}, // Channel 2 (index 1) is used
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_DISABLED, NULL}
  },
  .cr2        = 0,
  .dier       = 0,
};

// Pointer to the PWM driver instance to use
PWMDriver * const bsp_servo_pwm_driver = &PWMD1;
