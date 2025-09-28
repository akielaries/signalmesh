#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"


#define DAC_BUFFER_SIZE 400


static dacsample_t dac_buffer[DAC_BUFFER_SIZE] __attribute__((aligned(32)));


void make_sine_wave(dacsample_t* buffer, size_t size) {
  const float amplitude = 2047.0;  // Half of 4095 (max 12-bit)
  const float offset = 2047.0;     // Center offset
  const float two_pi = 6.283185307179586;

  for (size_t i = 0; i < size; i++) {
    float angle = two_pi * i / size;
    float value = offset + amplitude * sin(angle);
    buffer[i] = (dacsample_t)(value + 0.5); // round to nearest integer
  }
}

void make_square_wave(dacsample_t* buffer, size_t size) {
  const dacsample_t high = 4095;  // Maximum 12-bit value
  const dacsample_t low = 0;      // Minimum value

  for (size_t i = 0; i < size; i++) {
    // First half of the cycle: high, second half: low
    if (i < size / 2) {
      buffer[i] = high;
    } else {
      buffer[i] = low;
    }
  }
}

void make_sawtooth_wave(dacsample_t* buffer, size_t size) {
  const dacsample_t max_value = 4095;  // Maximum 12-bit value

  for (size_t i = 0; i < size; i++) {
    // Linear ramp from 0 to max_value
    buffer[i] = (dacsample_t)((max_value * i) / size);
  }
}

void make_triangle_wave(dacsample_t* buffer, size_t size) {
  const dacsample_t max_value = 4095;  // Maximum 12-bit value
  size_t half_size = size / 2;

  for (size_t i = 0; i < size; i++) {
    if (i < half_size) {
      // Rising edge: 0 to max_value
      buffer[i] = (dacsample_t)((max_value * i) / half_size);
    } else {
      // Falling edge: max_value to 0
      buffer[i] = (dacsample_t)(max_value - (max_value * (i - half_size)) / half_size);
    }
  }
}



/*
 * DAC streaming callback.
 */
size_t nx = 0, ny = 0, nz = 0;
static void end_cb1(DACDriver *dacp) {

  nz++;
  if (dacIsBufferComplete(dacp)) {
    nx += DAC_BUFFER_SIZE / 2;
  }
  else {
    ny += DAC_BUFFER_SIZE / 2;
  }

  if ((nz % 1000) == 0) {
    palToggleLine(PORTAB_LINE_LED1);
  }
}

/*
 * DAC error callback.
 */
static void error_cb1(DACDriver *dacp, dacerror_t err) {

  (void)dacp;
  (void)err;

  chSysHalt("DAC failure");
}

static const DACConfig dac1cfg1 = {
  .init         = 2047U,
  .datamode     = DAC_DHRM_12BIT_RIGHT,
  .cr           = 0
};

static const DACConversionGroup dacgrpcfg1 = {
  .num_channels = 1U,
  .end_cb       = end_cb1,
  .error_cb     = error_cb1,
  .trigger      = DAC_TRG(PORTAB_DAC_TRIG)
};

/*
 * GPT6 configuration.
 */
static const GPTConfig gpt6cfg1 = {
  //.frequency    = 6000000U,
  .frequency    = 7500000U,
  .callback     = NULL,
  .cr2          = TIM_CR2_MMS_1,    /* MMS = 010 = TRGO on Update Event.    */
  .dier         = 0U
};


void play_waveform(float frequency_hz) {
    const uint32_t num_samples = DAC_BUFFER_SIZE;
    uint32_t dac_update_rate = (uint32_t)(frequency_hz * num_samples);

    chprintf(chp, "Setting waveform frequency: %.2f Hz (DAC update rate: %u Hz)\r\n",
             frequency_hz, dac_update_rate);

    gptStopTimer(&GPTD6);
    gptChangeInterval(&GPTD6, gpt6cfg1.frequency / dac_update_rate);
    dacStartConversion(&DACD1, &dacgrpcfg1,
                       (dacsample_t *)dac_buffer, DAC_BUFFER_SIZE);
    gptStartContinuous(&GPTD6, gpt6cfg1.frequency / dac_update_rate);
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
  chprintf(chp, "System tick freq: %u Hz\r\n", CH_CFG_ST_FREQUENCY);
  chprintf(chp, "GPT6 clock: %u Hz\r\n",gpt6cfg1.frequency);

  chprintf(chp, "Setting GPIOA PIN 3-5 modes\r\n");
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);

  chprintf(chp, "Blinking on boot\r\n");
  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  chprintf(chp, "Creating sine wave DAC buffer\r\n");
  //make_sine_wave(dac_buffer, DAC_BUFFER_SIZE);
  make_square_wave(dac_buffer, DAC_BUFFER_SIZE);
  //make_sawtooth_wave(dac_buffer, DAC_BUFFER_SIZE);
  //make_triangle_wave(dac_buffer, DAC_BUFFER_SIZE);
  chprintf(chp, "Cleaning DCACHE\r\n");
  SCB_CleanDCache_by_Addr((uint32_t*)dac_buffer, DAC_BUFFER_SIZE * sizeof(dacsample_t));
  /* Starting DAC1 driver.*/
  chprintf(chp, "Starting DAC1 driver\r\n");
  dacStart(&DACD1, &dac1cfg1);

  /* Starting GPT6 driver, it is used for triggering the DAC.*/
  chprintf(chp, "Starting GPT6 driver\r\n");
  gptStart(&GPTD6, &gpt6cfg1);

  /* Starting a continuous conversion.*/
  chprintf(chp, "Starting DAC conversion\r\n");
  dacStartConversion(&DACD1, &dacgrpcfg1,
                     (dacsample_t *)dac_buffer, DAC_BUFFER_SIZE);

  float frequency_hz = 200.0f;
  const uint32_t num_samples = DAC_BUFFER_SIZE;
  uint32_t dac_update_rate = (uint32_t)(frequency_hz * num_samples);

  chprintf(chp, "freq: %f hz\r\n", frequency_hz);
  chprintf(chp, "samples: %ld\r\n", num_samples);
  chprintf(chp, "DAC rate: %ld\r\n", dac_update_rate);



  gptStartContinuous(&GPTD6, gpt6cfg1.frequency / dac_update_rate); // this will change the freq

  /*
   * Normal main() thread activity, if the button is pressed then the DAC
   * transfer is stopped.
   */
  while (true) {
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      gptStopTimer(&GPTD6);
      dacStopConversion(&DACD1);
    }
    chThdSleepMilliseconds(500);
    //chprintf(chp, "arggg....\r\n");
  }
  return 0;
}
