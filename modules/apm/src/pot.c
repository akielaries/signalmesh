#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"
#include "portab.h"

/*===========================================================================*/
/* ADC driver related.                                                       */
/*===========================================================================*/

#define ADC_GRP2_BUF_DEPTH      1   // one sample per callback for simplicity

/* Buffer for ADC samples */
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
adcsample_t samples2[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH)];

/* Latest ADC value */
volatile uint16_t raw_adc = 0;

/*===========================================================================*/
/* ADC callbacks                                                             */
/*===========================================================================*/

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
/* LED blinking thread                                                       */
/*===========================================================================*/

static THD_WORKING_AREA(waThread1, 512);
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

/*===========================================================================*/
/* Main                                                                      */
/*===========================================================================*/

int main(void) {
    halInit();
    chSysInit();

    palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
    palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
    sdStart(&SD5, &uart5_cfg);
    chprintf(chp, "\r\nStarting continuous ADC test...\r\n");

    /* Board setup */
    portab_setup();
    chprintf(chp, "counters - nx: %d ny: %d n: %d\r\n", nx, ny, n);

    /* Start LED blinking thread */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /* Start ADC driver */
    adcStart(&PORTAB_ADC1, &portab_adccfg1);

    /* No need to enable temperature sensor or VREF for potentiometer */
    // adcSTM32EnableVREF(&PORTAB_ADC1);
    // adcSTM32EnableTS(&PORTAB_ADC1);
  adcConvert(&PORTAB_ADC1, &portab_adcgrpcfg1, samples2, ADC_GRP2_BUF_DEPTH);
  cacheBufferInvalidate(samples2, sizeof (samples2) / sizeof (adcsample_t));

    /* Start GPT1 timer (used for triggering ADC) */
    gptStart(&PORTAB_GPT1, &portab_gptcfg1);

    /* Start continuous ADC conversion on PA0 */
    adcStartConversion(&PORTAB_ADC1, &portab_adcgrpcfg2,
                       samples2, ADC_GRP2_BUF_DEPTH);
    gptStartContinuous(&PORTAB_GPT1, 100U);

    /* Main loop: print latest ADC value safely */
    while (true) {
      for (size_t i = 0; i < ADC_GRP2_BUF_DEPTH; i++) {
        chprintf(chp, "ADC1[%lu]: %u\r\n", i, (unsigned)samples2[i]);
      }


        chprintf(chp, "Latest ADC: %u\r\n", raw_adc);
        chThdSleepMilliseconds(500);

        /* Stop ADC if button pressed */
        if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
            gptStopTimer(&PORTAB_GPT1);
            adcStopConversion(&PORTAB_ADC1);
        }
    }
}

