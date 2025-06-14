#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>

#define SAMPLES           100
#define AMPLITUDE         2047
#define OFFSET            2048


static dacsample_t sine_table[SAMPLES];
static dacsample_t stereo_table[SAMPLES * 2];

BaseSequentialStream* chp = (BaseSequentialStream*)&SD5;

static const DACConversionGroup dac_grp_cfg = {
  .num_channels = 2,
  .end_cb       = NULL,
  .error_cb     = NULL,
  .trigger      = true
};

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

void init_sine_table(void) {
  for (int i = 0; i < SAMPLES; i++) {
    float theta = 2.0f * M_PI * i / SAMPLES;
    sine_table[i] = (dacsample_t)(AMPLITUDE * sinf(theta) + OFFSET);
  }
}

void init_stereo_sine_table(void) {
  for (int i = 0; i < SAMPLES; i++) {
    float theta = 2.0f * M_PI * i / SAMPLES;
    dacsample_t sample = (dacsample_t)(AMPLITUDE * sinf(theta) + OFFSET);

    stereo_table[2 * i]     = sample; // DAC CH1 (Left)
    stereo_table[2 * i + 1] = sample; // DAC CH2 (Right)
  }
}

void setup_tim6_for_dac(uint32_t sample_rate_hz) {
  rccEnableTIM6(true);  // Enable clock for TIM6
  uint32_t timer_clk = STM32_TIMCLK1;
  uint32_t arr = timer_clk / sample_rate_hz;

  TIM6->PSC = 0;
  TIM6->ARR = arr - 1;
  TIM6->CR2 = TIM_CR2_MMS_1;   // TRGO on update event
  TIM6->CR1 = TIM_CR1_CEN;
}

int main(void) {
  halInit();
  chSysInit();

  // UART Debug
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8)); // USART5 TX
  palSetPadMode(GPIOD, GPIOD_PIN2,  PAL_MODE_ALTERNATE(8)); // USART5 RX
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Start of DMA sine wave generator...\r\n");

  // GPIO Setup
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);   // LED
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);  // DAC1 CH1
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);

  // Blink indicator
  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  init_stereo_sine_table();

  // Start DAC driver
  dacStart(&DACD1, &dac_cfg);

  // Setup timer for DAC trigger (TIM6 TRGO)
  float freq = 2000.0f;
  int sample_rate = (int)(freq * SAMPLES);  // 200 * 100 = 20 kHz
  setup_tim6_for_dac(sample_rate);

  // Enable DAC channel trigger and DMA (trigger source TIM6 TRGO)
  DAC1->CR = 0;  // Clear config
  DAC1->CR |= DAC_CR_TEN1 | DAC_CR_TSEL1_2 | DAC_CR_TSEL1_1 | DAC_CR_TSEL1_0 | DAC_CR_EN1 | DAC_CR_DMAEN1;
  DAC1->CR |= DAC_CR_TEN2 | DAC_CR_TSEL2_2 | DAC_CR_TSEL2_1 | DAC_CR_TSEL2_0 | DAC_CR_EN2 | DAC_CR_DMAEN2;

  chprintf(chp, "Configured %d Hz sine wave, SR = %d\r\n", (int)freq, sample_rate);

  // Start DMA transfer (stereo interleaved)
  dacStartConversion(&DACD1, &dac_grp_cfg, stereo_table, SAMPLES);

  while (true) {
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
