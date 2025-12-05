#pragma once

#include <stdint.h>
#include "bsp/configs/bsp_gpt_config.h" // For bsp_gpt_driver_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes a BSP GPT utility instance.
 * @param gptd Pointer to the bsp_gpt_driver_t object, representing the timer to use.
 */
void bsp_gpt_init(bsp_gpt_driver_t *gptd);

/**
 * @brief Starts a BSP GPT utility instance in continuous mode.
 * 
 * @param gptd Pointer to the bsp_gpt_driver_t object.
 * @param interval The timer interval in ticks.
 */
void bsp_gpt_start_continuous(bsp_gpt_driver_t *gptd, uint32_t interval);

/**
 * @brief Stops a BSP GPT utility timer.
 * @param gptd Pointer to the bsp_gpt_driver_t object.
 */
void bsp_gpt_stop(bsp_gpt_driver_t *gptd);


#ifdef __cplusplus
}
#endif
