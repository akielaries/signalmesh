#include "bsp/configs/bsp_gpt_config.h"

// Configuration for the GPT used to trigger the ADC
const GPTConfig bsp_gpt_adc_trigger_config = {
    .frequency = 20000U,
    .callback  = NULL,
    .cr2       = TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event */
    .dier      = 0U
};

// Instance of the GPT driver for the ADC trigger
bsp_gpt_driver_t bsp_gpt_adc_trigger = {
    .gptd = &GPTD4,
    .config = &bsp_gpt_adc_trigger_config
};
