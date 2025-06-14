#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>

#define SAMPLES   25                     // Fewer samples = higher frequency
#define AMPLITUDE 2047                   // 12-bit DAC, centered around 2048
#define OFFSET    2048


static dacsample_t left_wave[SAMPLES];
static dacsample_t right_wave[SAMPLES];


static const DACConfig dac_cfg = {
  .init     = 2048U,
  .datamode = DAC_DHRM_12BIT_RIGHT,
  //.cr       = 0,
  //.cr       = DAC_CR_TEN1 | DAC_CR_TEN2
  //.cr       = DAC_CR_TSEL1_0 | DAC_CR_TEN1 |  // DAC1_CH1 trigger
  //            DAC_CR_TSEL2_0 | DAC_CR_TEN2    // DAC1_CH2 trigger
};


static const DACConversionGroup dac_grpcfg = {
  .num_channels = 2,     // Stereo
  .end_cb       = NULL,
  .error_cb     = NULL,
  .trigger      = true
};

static const DACConversionGroup single_channel_cfg = {
  .num_channels = 1,
  .end_cb       = NULL,
  .error_cb     = NULL,
  .trigger      = true // You still need TIM6 running for both
};

static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

void init_stereo_wave(void) {
  for (int i = 0; i < SAMPLES; i++) {
    float theta = 2.0f * M_PI * i / SAMPLES;
    left_wave[i]  = (dacsample_t)(AMPLITUDE * sinf(theta) + OFFSET);
    right_wave[i] = (dacsample_t)(AMPLITUDE * sinf(theta) + OFFSET); // Or vary for stereo effect
  }
}

// Timer triggers DAC conversion at fixed rate
void setup_tim6_for_dac(void) {
  rccEnableTIM6(true);
  TIM6->PSC = 169;               // 170 MHz / (169+1) = 1 MHz
  TIM6->ARR = 100;                // 1 MHz / 22 ≈ 45.5 kHz sample rate
  TIM6->CR2 = TIM_CR2_MMS_1;     // TRGO on update event
  TIM6->CR1 |= TIM_CR1_CEN;
}

void dac_write(int dac, int16_t val) {
  switch(dac){
  case 1:
    dacPutChannelX(&DACD1, 0, val);
    break;
  case 2:
    dacPutChannelX(&DACD2, 0, val);
    break;
  default:
    break;
  }
}

int main(void) {
  halInit();
  chSysInit();

  // Configure serial TX/RX for debugging
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8)); // USART5 TX
  palSetPadMode(GPIOD, GPIOD_PIN2,  PAL_MODE_ALTERNATE(8)); // USART5 RX
  palSetPadMode(GPIOG, GPIOG_PIN3, PAL_MODE_INPUT); // ext push button
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT); // blue LED?

  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG); // DAC1_CH1
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG); // DAC1_CH2

  sdStart(&SD5, &uart5_cfg);
  BaseSequentialStream* chp = (BaseSequentialStream*) &SD5;
  chprintf(chp, "start of program...\r\n");

  // Flash LED to indicate startup
  for (uint8_t i = 0; i < 10; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
  }

  // Init waveform, DAC, and trigger
  init_stereo_wave();
  dacStart(&DACD1, &dac_cfg);
  dacStart(&DACD2, &dac_cfg);
/*
  setup_tim6_for_dac();
  dacStartConversion(&DACD1, &single_channel_cfg, left_wave, SAMPLES);
  dacStartConversion(&DACD2, &single_channel_cfg, right_wave, SAMPLES);
*/
  // debug DAC voltage
  // dacPutChannelX(&DACD1, 0, 1024);
  // dacPutChannelX(&DACD2, 0, 3072);

//  dac_write(1, 4095); // Write mid-scale to PA4
//  dac_write(2, 4095); // Write some value to PA5
int i = 0;
while (true) {
  float theta = 2.0f * M_PI * i / 100.0f;
  int16_t val = (int16_t)(2047 * sinf(theta) + 2048);
  dac_write(1, val);
  dac_write(2, val);
  i = (i + 1) % 100;
  //chThdSleepMilliseconds(1); // ~1 kHz update rate
}

  // Debug loop
  int counter = 0;
  while (true) {
    chThdSleepMilliseconds(1000);
    chprintf(chp, "Uptime: %lu ticks, counter: %d\r\n",
             chVTGetSystemTimeX(),
             counter++);
  }
}

