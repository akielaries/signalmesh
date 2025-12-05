#pragma once

#include "hal.h"

// Opaque pointer to a GPT driver configuration for BSP internal use
typedef struct bsp_gpt_driver_t {
    GPTDriver *gptd;
    const GPTConfig *config;
} bsp_gpt_driver_t;

extern const GPTConfig bsp_gpt_adc_trigger_config;
extern bsp_gpt_driver_t bsp_gpt_adc_trigger;
