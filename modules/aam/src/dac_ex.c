#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>


#define SAMPLES           100
#define AMPLITUDE         2047
#define OFFSET            2048
#define SYSTEM_FREQ_HZ    1000000  // 1 MHz virtual timer base


static dacsample_t sine_table[SAMPLES];


BaseSequentialStream* chp = (BaseSequentialStream*)&SD5;


static const DACConfig dac_cfg = {
  .init     = 2048U,
  .datamode = DAC_DHRM_12BIT_RIGHT,
};


static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};


void delay_us_approx(uint32_t us) {
  // Tweak this loop constant for your MCU speed
  for (volatile uint32_t i = 0; i < (us * 10); i++) {
    __asm__ volatile ("nop");
  }
}

// Initialize sine table
void init_sine_table(void) {
  for (int i = 0; i < SAMPLES; i++) {
    float theta = 2.0f * M_PI * i / SAMPLES;
    sine_table[i] = (dacsample_t)(AMPLITUDE * sinf(theta) + OFFSET);
  }
}

/*
// Output sine wave of desired frequency (in Hz)
void play_sine_wave(float frequency_hz) {
  int i = 0;
  // This is how often each sample must be output (us)
  float sample_interval_us = 1000000.0f / (frequency_hz * SAMPLES);
  systime_t period_ticks = TIME_US2I((uint32_t)sample_interval_us);

  systime_t next = chVTGetSystemTimeX();
  while (true) {
    dacPutChannelX(&DACD1, 0, sine_table[i]);
    dacPutChannelX(&DACD2, 0, sine_table[i]);
    i = (i + 1) % SAMPLES;
    next += period_ticks;
    chThdSleepUntil(next);
  }
}
*/
void play_sine_wave(float frequency_hz) {
  int i = 0;
  float sample_interval_us = 1000000.0f / (frequency_hz * SAMPLES);

  while (true) {
    //chprintf(chp, "writing %d to DAC1\r\n", sine_table[i]);
    //chprintf(chp, "writing %d to DAC2\r\n", sine_table[i]);
    dacPutChannelX(&DACD1, 0, sine_table[i]);
    dacPutChannelX(&DACD2, 0, sine_table[i]);
    i = (i + 1) % SAMPLES;
    delay_us_approx((uint32_t)sample_interval_us);
  }
}


void dac_write(int dac, int16_t val) {
  switch(dac) {
    case 1:
      dacPutChannelX(&DACD1, 0, val);
      break;
    case 2:
      dacPutChannelX(&DACD2, 0, val);
      break;
  }
}

int main(void) {
  halInit();
  chSysInit();

  // UART Debug setup
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8)); // USART5 TX
  palSetPadMode(GPIOD, GPIOD_PIN2,  PAL_MODE_ALTERNATE(8)); // USART5 RX
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "start of program...\r\n");

  // GPIO Setup
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);  // LED
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);  // DAC1_CH1
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);  // DAC1_CH2

  for (uint8_t i = 0; i < 10; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
  }

  init_sine_table();           // generate waveform table
  dacStart(&DACD1, &dac_cfg);  // start DAC channels
  dacStart(&DACD2, &dac_cfg);

  float frequency = 100.0f;
  play_sine_wave(frequency);

  return 0; // Unreachable
}

