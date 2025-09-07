#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"


/*===========================================================================*/
/* ADC driver related.                                                       */
/*===========================================================================*/


#define ADC_GRP1_BUF_DEPTH      1
#define ADC_GRP2_BUF_DEPTH      64
#define TS_CAL1_ADDR            ((uint16_t*)0x1FF1E820)
#define TS_CAL2_ADDR            ((uint16_t*)0x1FF1E840)
#define VREFINT_CAL_ADDR        ((uint16_t*)0x1FF1E860)
// Parameters for a 10k NTC, Beta = 3950
#define THERMISTOR_NOMINAL   10000.0f  // resistance at 25 °C
#define TEMPERATURE_NOMINAL  25.0f     // reference temperature
#define B_COEFFICIENT        3950.0f   // Beta constant
#define SERIES_RESISTOR      10000.0f  // your fixed resistor



float adc_to_steinhart(uint16_t adc_value) {
  float v_ratio = (float)adc_value / 65535.0f;   // for 16-bit ADC
  float r_therm = SERIES_RESISTOR * (1.0f / v_ratio - 1.0f);

  float steinhart;
  steinhart = r_therm / THERMISTOR_NOMINAL;      // (R/R0)
  steinhart = logf(steinhart);                   // ln(R/R0)
  steinhart /= B_COEFFICIENT;                    // 1/B * ln(R/R0)
  steinhart += 1.0f / (TEMPERATURE_NOMINAL + 273.15f); // + (1/T0)
  steinhart = 1.0f / steinhart;                  // invert
  steinhart -= 273.15f;                          // Kelvin -> °C
  return steinhart;
}

/*
float adc_to_vsense(uint16_t ts_data) {
  const float ts_cal1_temp = 30.0f;
  const float ts_cal2_temp = 130.0f;

  uint16_t ts_cal1 = *TS_CAL1_ADDR;   // factory calib @ 30 °C
  uint16_t ts_cal2 = *TS_CAL2_ADDR;   // factory calib @ 130 °C
  ts_cal1 = (*TS_CAL1_ADDR);     // scale 12?-bit value to 16-bit? comes in as 16 bit anyways...
  ts_cal2 = (*TS_CAL2_ADDR);

  float temp = ts_cal1_temp +
               ((float)(ts_data - ts_cal1)) *
                (ts_cal2_temp - ts_cal1_temp) /
                (float)(ts_cal2 - ts_cal1);
  return temp;
}
*/

float adc_to_vsense(uint16_t ts_data) {
  const float ts_cal1_temp = 30.0f;
  const float ts_cal2_temp = 130.0f;

  uint16_t ts_cal1 = *TS_CAL1_ADDR >> 4;  // convert to 12-bit
  uint16_t ts_cal2 = *TS_CAL2_ADDR >> 4;
  uint16_t ts_data_12bit = ts_data >> 4;

  float temp = ts_cal1_temp +
               ((float)(ts_data_12bit - ts_cal1)) *
                (ts_cal2_temp - ts_cal1_temp) /
                (float)(ts_cal2 - ts_cal1);

  return temp;
}


float adc_to_vdda(uint16_t vref_data) {
  // Factory calibration constant (12-bit stored in 16-bit word).
  uint32_t vrefint_cal = (uint32_t)(*VREFINT_CAL_ADDR);// << 4; // scale to 16-bit space

  if (vref_data == 0) return 0.0f; // avoid div/0

  // Factory reference voltage for calibration is 3.3 V on most STM32s.
  // Check your part’s datasheet: some L4/L5 parts use 3.0 V.
  const float VREFINT_CAL_VOLT = 3.3f;

  float vdda = VREFINT_CAL_VOLT * ((float)vrefint_cal / (float)vref_data);
  return vdda;
}

/* Buffers are allocated with size and address aligned to the cache
   line size.*/
//adcsample_t samples1[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH)];

#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
adcsample_t samples2[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH)];
adcsample_t samples3[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP3_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH)];


uint16_t adc_value_a(int ix) {
  if (ix < 0 || ix >= ADC_GRP2_NUM_CHANNELS) return 0;
  return (int16_t)samples2[ix];
}

uint16_t adc_value_b(int ix) {
  if (ix < 0 || ix >= ADC_GRP3_NUM_CHANNELS) return 0;
  return (int16_t)samples3[ix];
}

/*
 * ADC streaming callback.
 */
size_t n= 0, nx = 0, ny = 0;
void adccallback(ADCDriver *adcp) {

  /* Updating counters.*/
  n++;
  if (adcIsBufferComplete(adcp)) {
    nx += 1;
  }
  else {
    ny += 1;
  }

  if ((n % 200) == 0U) {
#if defined(PORTAB_LINE_LED2)
    palToggleLine(PORTAB_LINE_LED2);
#endif
  }
}

/*
 * ADC errors callback, should never happen.
 */
void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;

  chSysHalt("it happened");
}

/*===========================================================================*/
/* Application code.                                                         */
/*===========================================================================*/

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED attached to TP1.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
    palClearLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /* Board-dependent GPIO setup code.*/
  portab_setup();
  chprintf(chp, "\r\n...Starting...\r\n\r\n");

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Starting PORTAB_ADC1 driver and the temperature sensor.
   */
  adcStart(&PORTAB_ADC1, &portab_adccfg1);
  adcStart(&PORTAB_ADC3, &portab_adccfg1);
  adcSTM32EnableVREF(&PORTAB_ADC3);
  adcSTM32EnableTS(&PORTAB_ADC3);

  /*
   * Starting PORTAB_GPT1 driver, it is used for triggering the ADC.
   */
  gptStart(&PORTAB_GPT1, &portab_gptcfg1);

  /*
   * Starting an ADC continuous conversion triggered with a period of
   * 1/10000 second.
   */
  adcStartConversion(&PORTAB_ADC1, &portab_adcgrpcfg2,
                     samples2, ADC_GRP2_BUF_DEPTH);
  adcStartConversion(&PORTAB_ADC3, &portab_adcgrpcfg3,
                     samples3, ADC_GRP2_BUF_DEPTH);

  gptStartContinuous(&PORTAB_GPT1, 100U);

  //chprintf(chp, "ADC1[%lu]: %d\r\n", 0, (unsigned)samples1[0]);

  /*
   * Normal main() thread activity, if the button is pressed then the
   * conversion is stopped.
   */
  while (true) {
    cacheBufferInvalidate(samples2, sizeof(samples2)/sizeof(adcsample_t));
    cacheBufferInvalidate(samples3, sizeof(samples3)/sizeof(adcsample_t));
    int pot1, pot2, pot3, therm1, v_sense, v_ref;

    // probably worth some enums huh
    v_sense = adc_value_b(0);
    v_ref = adc_value_b(1);

    pot1 = adc_value_a(0);
    pot2 = adc_value_a(1);
    pot3 = adc_value_a(2);
    therm1 = adc_value_a(3);

/*
    chprintf(chp, "Full buffer:\r\n");
    for (uint32_t i = 0; i < ADC_GRP2_BUF_DEPTH; i++) {
      chprintf(chp, "%d\r\n", samples2[i]);
    }
*/
    chprintf(chp, "Pot1: %d  Pot2: %d Pot3: %d \r\n",
             pot1,
             pot2,
             pot3);

    chprintf(chp, "IntVolt: %d\r\n", v_ref);

    chprintf(chp, "Therm1: %d (%f C / %f F) V_SENSE: %d (%f C / %f F)\r\n\r\n",
             therm1,
             adc_to_steinhart(therm1),
             ((adc_to_steinhart(therm1) * 9 / 5) + 32),
             v_sense,
             adc_to_vsense(v_sense),
             ((adc_to_vsense(v_sense) * 9 / 5) + 32));

    chprintf(chp, "Raw TS: %d, Cal1: %d, Cal2: %d\r\n",
        v_sense, *TS_CAL1_ADDR, *TS_CAL2_ADDR);
    chprintf(chp, "Raw VREF: %d VREF_CAL: %d VDDA: %f V\r\n",
                  v_ref,
                  *VREFINT_CAL_ADDR,
                  adc_to_vdda(v_ref));

    //chprintf(chp, "counters - nx: %d ny: %d n: %d\r\n", nx, ny, n);

    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      gptStopTimer(&PORTAB_GPT1);
      adcStopConversion(&PORTAB_ADC1);
    }
    chThdSleepMilliseconds(500);
  }

  return 0;
}
