/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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
#define TS_CAL1_ADDR ((uint16_t*)0x1FF1E820)
#define TS_CAL2_ADDR ((uint16_t*)0x1FF1E840)


float adc_to_temperature(uint16_t ts_data) {
    const float ts_cal1_temp = 30.0f;    // °C
    const float ts_cal2_temp = 130.0f;   // °C

    uint16_t ts_cal1 = *TS_CAL1_ADDR;    // raw ADC at 30°C
    uint16_t ts_cal2 = *TS_CAL2_ADDR;    // raw ADC at 130°C
    ts_cal1 = (*TS_CAL1_ADDR) << 4;  // scale 12-bit value to 16-bit
    ts_cal2 = (*TS_CAL2_ADDR) << 4;

    //float seq1 = (ts_cal2 - ts_cal1);

    float temp = ts_cal1_temp +
                 ((float)(ts_data - ts_cal1)) * (ts_cal2_temp - ts_cal1_temp) / (float)(ts_cal2 - ts_cal1);
    return temp;
}


/* Buffers are allocated with size and address aligned to the cache
   line size.*/
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
//adcsample_t samples1[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH)];

#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
adcsample_t samples2[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH)];


uint16_t adc_value(int ix) {
  if (ix < 0 || ix >= ADC_GRP2_NUM_CHANNELS) return 0;
  return (int16_t)samples2[ix];
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
  adcSTM32EnableVREF(&PORTAB_ADC1);
  adcSTM32EnableTS(&PORTAB_ADC1);

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

  gptStartContinuous(&PORTAB_GPT1, 100U);

  //chprintf(chp, "ADC1[%lu]: %u\r\n", 0, (unsigned)samples1[0]);

  /*
   * Normal main() thread activity, if the button is pressed then the
   * conversion is stopped.
   */
  while (true) {
    cacheBufferInvalidate(samples2, sizeof(samples2)/sizeof(adcsample_t));
    int pot1, pot2, pot3, therm1, intrn_temp;

    // probably worth some enums huh
    intrn_temp = adc_value(0);
    pot1 = adc_value(1);
    pot2 = adc_value(2);
    pot3 = adc_value(3);
    therm1 = adc_value(4);

/*
    chprintf(chp, "Full buffer:\r\n");
    for (uint32_t i = 0; i < ADC_GRP2_BUF_DEPTH; i++) {
      chprintf(chp, "%u\r\n", samples2[i]);
    }
*/
    chprintf(chp, "Pot1: %u  Pot2: %u Pot3: %u \r\n",
             pot1,
             pot2,
             pot3);

    chprintf(chp, "Therm1: %u (%f C / %f F) IntTemp: %u (%f C / %f F)\r\n\r\n",
             therm1,
             adc_to_temperature(therm1),
             ((adc_to_temperature(therm1) * 9 / 5) + 32),
             intrn_temp,
             adc_to_temperature(intrn_temp),
             ((adc_to_temperature(intrn_temp) * 9 / 5) + 32));

    chprintf(chp, "Raw TS: %u, Cal1: %u, Cal2: %u\r\n",
        intrn_temp, *TS_CAL1_ADDR, *TS_CAL2_ADDR);

    //chprintf(chp, "counters - nx: %d ny: %d n: %d\r\n", nx, ny, n);

    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      gptStopTimer(&PORTAB_GPT1);
      adcStopConversion(&PORTAB_ADC1);
    }
    chThdSleepMilliseconds(500);
  }

  return 0;
}
