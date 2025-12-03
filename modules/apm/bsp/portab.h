/**
 * @file    portab.h
 * @brief   Application portability macros and structures.
 */

#ifndef PORTAB_H
#define PORTAB_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define PORTAB_LINE_LED1      LINE_LED1
#define PORTAB_LINE_LED2      LINE_LED2
#define PORTAB_LINE_LED3      LINE_LED3
#define PORTAB_LED_OFF        PAL_LOW
#define PORTAB_LED_ON         PAL_HIGH
#define PORTAB_LINE_BUTTON    LINE_BUTTON
#define PORTAB_BUTTON_PRESSED PAL_HIGH

#define PORTAB_SD1 SD3

#define PORTAB_DAC_TRIG 5

#define PORTAB_GPT1 GPTD4
#define PORTAB_ADC1 ADCD1
#define PORTAB_ADC3 ADCD3

#define ADC_GRP1_NUM_CHANNELS 1
#define ADC_GRP2_NUM_CHANNELS 4
#define ADC_GRP3_NUM_CHANNELS 2


/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/
// TODO these should be more verbose
extern const GPTConfig portab_gptcfg1;
extern const ADCConfig portab_adccfg1;
extern const ADCConversionGroup portab_adcgrpcfg1;
extern const ADCConversionGroup portab_adcgrpcfg2;
extern const ADCConversionGroup portab_adcgrpcfg3;
extern const PWMConfig portabpwmgrpcfg1;

extern BaseSequentialStream *chp;

#ifdef __cplusplus
extern "C" {
#endif
void portab_setup(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* PORTAB_H */

/** @} */
