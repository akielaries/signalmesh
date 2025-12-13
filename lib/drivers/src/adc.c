#include "drivers/adc.h"
#include "bsp/configs/bsp_adc_config.h"
#include "bsp/utils/bsp_gpt.h"
#include "hal.h" // For adcStart, palSetPadMode etc.
#include <string.h>

void adc_init(void) {
  // Configure GPIOs for analog inputs
  for (uint32_t i = 0; i < bsp_adc_pins_num; i++) {
    palSetPadMode(bsp_adc_pins[i].port,
                  bsp_adc_pins[i].pad,
                  PAL_MODE_INPUT_ANALOG);
  }

  // Start ADC peripherals (handling multiple groups on the same peripheral)
  for (int i = 0; i < ADC_GROUP_COUNT; i++) {
    bool already_started = false;
    for (int j = 0; j < i; j++) {
      if (adc_groups[i].adcd == adc_groups[j].adcd) {
        already_started = true;
        break;
      }
    }
    if (!already_started) {
      adcStart(adc_groups[i].adcd, &bsp_adc_config);
    }
  }

  // Enable internal sensors, assuming they are on a specific peripheral
  // A more robust implementation might look this up from the BSP config
  adcSTM32EnableVREF(&ADCD3);
  adcSTM32EnableTS(&ADCD3);

  // Initialize the GPT utility that triggers the ADC
  bsp_gpt_init(&bsp_gpt_adc_trigger);
}

void adc_start_continuous(uint32_t group_mask) {
  for (int i = 0; i < ADC_GROUP_COUNT; i++) {
    if (group_mask & (1 << i)) {
      adcStartConversion(adc_groups[i].adcd,
                         adc_groups[i].config,
                         adc_groups[i].buffer,
                         adc_groups[i].buffer_depth);
    }
  }
  // Start the timer with an interval of 100 ticks (from original adc.c)
  bsp_gpt_start_continuous(&bsp_gpt_adc_trigger, 100U);
}

void adc_stop_continuous(uint32_t group_mask) {
  bsp_gpt_stop(&bsp_gpt_adc_trigger);
  for (int i = 0; i < ADC_GROUP_COUNT; i++) {
    if (group_mask & (1 << i)) {
      adcStopConversion(adc_groups[i].adcd);
    }
  }
}

uint32_t
adc_get_samples(adc_group_id_t group_id, uint16_t *buffer, uint32_t len) {
  if (group_id >= ADC_GROUP_COUNT || buffer == NULL) {
    return 0;
  }

  const adc_group_t *group = &adc_groups[group_id];
  uint32_t num_channels    = group->config->num_channels;
  uint32_t copy_len        = len < num_channels ? len : num_channels;

  cacheBufferInvalidate(group->buffer,
                        group->buffer_depth * num_channels *
                          sizeof(adcsample_t));

  // For simplicity, we copy just the first sample of each channel.
  memcpy(buffer, group->buffer, copy_len * sizeof(uint16_t));

  return copy_len;
}
