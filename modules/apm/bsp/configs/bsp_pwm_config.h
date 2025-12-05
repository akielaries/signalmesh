#pragma once

#include "hal.h"

// Servo-specific pulse widths
#define SERVO_MIN_US 600
#define SERVO_MAX_US 2400

// Define GPIO for the servo PWM
#define BSP_SERVO_PWM_DRIVER        PWMD1
#define BSP_SERVO_PWM_CHANNEL       1
#define BSP_SERVO_GPIO_PORT         GPIOE
#define BSP_SERVO_GPIO_PIN          11
#define BSP_SERVO_GPIO_ALT_MODE     1

extern const PWMConfig bsp_servo_pwm_config;
extern PWMDriver * const bsp_servo_pwm_driver;
