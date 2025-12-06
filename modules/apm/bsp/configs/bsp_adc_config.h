#pragma once

#include "hal.h"
#include "drivers/adc.h" // Include adc.h for adc_group_id_t definition

typedef struct {
    ioportid_t port;
    uint16_t pad;
} bsp_pin_t;

// This struct defines a complete ADC conversion group for the HAL interface
typedef struct {
    ADCDriver *adcd;
    const ADCConversionGroup *config;
    adcsample_t *buffer;
    uint32_t buffer_depth;
} adc_group_t;

// Extern declarations for the ADC groups defined in the BSP
extern const adc_group_t adc_groups[ADC_GROUP_COUNT];
extern const ADCConfig bsp_adc_config;
extern const bsp_pin_t bsp_adc_pins[];
extern const uint32_t bsp_adc_pins_num;
