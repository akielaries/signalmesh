#include "bsp/configs/bsp_adc_config.h"

#define ADC_POTS_NUM_CHANNELS       4
#define ADC_INTERNAL_NUM_CHANNELS   2
#define ADC_BUFFER_DEPTH            64

// ADC callbacks (can be generic or specific)
static void adccallback(ADCDriver *adcp) { (void)adcp; }
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) { (void)adcp; (void)err; chSysHalt("ADC error"); }

// Buffers
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
static adcsample_t pot_samples[ADC_POTS_NUM_CHANNELS * ADC_BUFFER_DEPTH];

#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
static adcsample_t internal_samples[ADC_INTERNAL_NUM_CHANNELS * ADC_BUFFER_DEPTH];

// ADC conversion group for potentiometers and thermistor
static const ADCConversionGroup adc_grp_pots_config = {
  .circular     = true,
  .num_channels = ADC_POTS_NUM_CHANNELS,
  .end_cb       = adccallback,
  .error_cb     = adcerrorcallback,
  .cfgr         = ADC_CFGR_CONT_ENABLED | ADC_CFGR_RES_16BITS,
  .pcsel        = ADC_SELMASK_IN10 | ADC_SELMASK_IN13 | ADC_SELMASK_IN15 | ADC_SELMASK_IN5,
  .smpr         = {
    ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5),
    ADC_SMPR2_SMP_AN10(ADC_SMPR_SMP_384P5) | ADC_SMPR2_SMP_AN13(ADC_SMPR_SMP_384P5) | ADC_SMPR2_SMP_AN15(ADC_SMPR_SMP_384P5),
  },
  .sqr          = {
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN10) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN13) | ADC_SQR1_SQ3_N(ADC_CHANNEL_IN15) | ADC_SQR1_SQ4_N(ADC_CHANNEL_IN5),
    0, 0, 0
  }
};

// ADC conversion group for internal sensors
static const ADCConversionGroup adc_grp_internal_config = {
  .circular     = true,
  .num_channels = ADC_INTERNAL_NUM_CHANNELS,
  .end_cb       = adccallback,
  .error_cb     = adcerrorcallback,
  .cfgr         = ADC_CFGR_CONT_ENABLED | ADC_CFGR_RES_16BITS,
  .pcsel        = ADC_SELMASK_IN18 | ADC_SELMASK_IN19,
  .smpr         = { 0, ADC_SMPR2_SMP_AN18(ADC_SMPR_SMP_384P5) | ADC_SMPR2_SMP_AN19(ADC_SMPR_SMP_384P5) },
  .sqr          = { ADC_SQR1_SQ1_N(ADC_CHANNEL_IN18) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN19), 0, 0, 0 }
};

// Array holding all the ADC group configurations for the project
const adc_group_t adc_groups[ADC_GROUP_COUNT] = {
    [ADC_GROUP_POTS] = {
        .adcd = &ADCD1,
        .config = &adc_grp_pots_config,
        .buffer = pot_samples,
        .buffer_depth = ADC_BUFFER_DEPTH
    },
    [ADC_GROUP_INTERNAL] = {
        .adcd = &ADCD3,
        .config = &adc_grp_internal_config,
        .buffer = internal_samples,
        .buffer_depth = ADC_BUFFER_DEPTH
    }
};

// Common ADC peripheral configuration
const ADCConfig bsp_adc_config = {
  .difsel       = 0U,
  .calibration  = 0U
};

// GPIO pin definitions for the ADC channels
const bsp_pin_t bsp_adc_pins[] = {
    {GPIOB, 1}, // PB1, ADC channel 5 (thermistor)
    {GPIOA, 3}, // PA3, ADC channel 15 (pot 3)
    {GPIOC, 0}, // PC0, ADC channel 10 (pot 1)
    {GPIOC, 3}  // PC3, ADC channel 13 (pot 2)
};
const uint32_t bsp_adc_pins_num = sizeof(bsp_adc_pins) / sizeof(bsp_pin_t);
