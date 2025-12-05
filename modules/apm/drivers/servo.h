#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the servo motor driver.
 * @note This function configures the necessary GPIOs and starts the PWM driver.
 */
void servo_init(void);

/**
 * @brief Sets the servo angle in degrees.
 * 
 * @param degrees The desired angle, from 0 to 180.
 */
void servo_set_angle(float degrees);

/**
 * @brief Sets the raw pulse width for the servo.
 * 
 * @param us The pulse width in microseconds.
 */
void servo_set_pos(uint16_t us);

/**
 * @brief Disables the servo PWM channel.
 */
void servo_stop(void);


#ifdef __cplusplus
}
#endif
