#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define abstract identifiers for the ADC groups
typedef enum {
  ADC_GROUP_POTS,
  ADC_GROUP_INTERNAL,
  ADC_GROUP_COUNT
} adc_group_id_t;

/**
 * @brief Initializes the ADC driver and configures the necessary GPIOs.
 */
void adc_init(void);

/**
 * @brief Starts continuous ADC conversions on specified groups.
 *
 * @param group_mask A bitmask of the adc_group_id_t to start.
 */
void adc_start_continuous(uint32_t group_mask);

/**
 * @brief Stops continuous ADC conversions on specified groups.
 * @param group_mask A bitmask of the adc_group_id_t to stop.
 */
void adc_stop_continuous(uint32_t group_mask);

/**
 * @brief Copies the latest ADC sample values for a specific group.
 *
 * @param group_id The ID of the group to get samples from.
 * @param[out] buffer The buffer to store the samples in.
 * @param len The length of the buffer.
 * @return The number of samples copied.
 */
uint32_t
adc_get_samples(adc_group_id_t group_id, uint16_t *buffer, uint32_t len);


#ifdef __cplusplus
}
#endif
