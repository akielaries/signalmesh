#include <math.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "portab.h"

#define DAC_BUFFER_SIZE 360


static dacsample_t dac_buffer[DAC_BUFFER_SIZE] __attribute__((aligned(32)));
//__attribute__((section(".dma_buffer")))
//dacsample_t dac_buffer[DAC_BUFFER_SIZE];

static const dacsample_t static_dac_buffer[DAC_BUFFER_SIZE] = {
  2047, 2082, 2118, 2154, 2189, 2225, 2260, 2296, 2331, 2367, 2402, 2437,
  2472, 2507, 2542, 2576, 2611, 2645, 2679, 2713, 2747, 2780, 2813, 2846,
  2879, 2912, 2944, 2976, 3008, 3039, 3070, 3101, 3131, 3161, 3191, 3221,
  3250, 3278, 3307, 3335, 3362, 3389, 3416, 3443, 3468, 3494, 3519, 3544,
  3568, 3591, 3615, 3637, 3660, 3681, 3703, 3723, 3744, 3763, 3782, 3801,
  3819, 3837, 3854, 3870, 3886, 3902, 3917, 3931, 3944, 3958, 3970, 3982,
  3993, 4004, 4014, 4024, 4033, 4041, 4049, 4056, 4062, 4068, 4074, 4078,
  4082, 4086, 4089, 4091, 4092, 4093, 4094, 4093, 4092, 4091, 4089, 4086,
  4082, 4078, 4074, 4068, 4062, 4056, 4049, 4041, 4033, 4024, 4014, 4004,
  3993, 3982, 3970, 3958, 3944, 3931, 3917, 3902, 3886, 3870, 3854, 3837,
  3819, 3801, 3782, 3763, 3744, 3723, 3703, 3681, 3660, 3637, 3615, 3591,
  3568, 3544, 3519, 3494, 3468, 3443, 3416, 3389, 3362, 3335, 3307, 3278,
  3250, 3221, 3191, 3161, 3131, 3101, 3070, 3039, 3008, 2976, 2944, 2912,
  2879, 2846, 2813, 2780, 2747, 2713, 2679, 2645, 2611, 2576, 2542, 2507,
  2472, 2437, 2402, 2367, 2331, 2296, 2260, 2225, 2189, 2154, 2118, 2082,
  2047, 2012, 1976, 1940, 1905, 1869, 1834, 1798, 1763, 1727, 1692, 1657,
  1622, 1587, 1552, 1518, 1483, 1449, 1415, 1381, 1347, 1314, 1281, 1248,
  1215, 1182, 1150, 1118, 1086, 1055, 1024,  993,  963,  933,  903,  873,
   844,  816,  787,  759,  732,  705,  678,  651,  626,  600,  575,  550,
   526,  503,  479,  457,  434,  413,  391,  371,  350,  331,  312,  293,
   275,  257,  240,  224,  208,  192,  177,  163,  150,  136,  124,  112,
   101,   90,   80,   70,   61,   53,   45,   38,   32,   26,   20,   16,
    12,    8,    5,    3,    2,    1,    0,    1,    2,    3,    5,    8,
    12,   16,   20,   26,   32,   38,   45,   53,   61,   70,   80,   90,
   101,  112,  124,  136,  150,  163,  177,  192,  208,  224,  240,  257,
   275,  293,  312,  331,  350,  371,  391,  413,  434,  457,  479,  503,
   526,  550,  575,  600,  626,  651,  678,  705,  732,  759,  787,  816,
   844,  873,  903,  933,  963,  993, 1024, 1055, 1086, 1118, 1150, 1182,
  1215, 1248, 1281, 1314, 1347, 1381, 1415, 1449, 1483, 1518, 1552, 1587,
  1622, 1657, 1692, 1727, 1763, 1798, 1834, 1869, 1905, 1940, 1976, 2012
};


static BaseSequentialStream *chp = (BaseSequentialStream *)&SD5;
static const SerialConfig uart5_cfg = {
  .speed = 1000000, 
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};


void generate_dac_buffer(dacsample_t* buffer, size_t size) {
    const float amplitude = 2047.0;  // Half of 4095 (max 12-bit)
    const float offset = 2047.0;     // Center offset
    const float two_pi = 6.283185307179586;

    for (size_t i = 0; i < size; i++) {
        float angle = two_pi * i / size;
        float value = offset + amplitude * sin(angle);
        buffer[i] = (dacsample_t)(value + 0.5); // round to nearest integer
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
  .frequency    = 1000000U,
  .callback     = NULL,
  .cr2          = TIM_CR2_MMS_1,    /* MMS = 010 = TRGO on Update Event.    */
  .dier         = 0U
};


void play_sine_wave(float frequency_hz) {
    const uint32_t num_samples = DAC_BUFFER_SIZE;
    uint32_t dac_update_rate = (uint32_t)(frequency_hz * num_samples);

    chprintf(chp, "Setting sine wave frequency: %.2f Hz (DAC update rate: %u Hz)\r\n",
             frequency_hz, dac_update_rate);

    gptStopTimer(&GPTD6);
    gptChangeInterval(&GPTD6, gpt6cfg1.frequency / dac_update_rate);
    dacStartConversion(&DACD1, &dacgrpcfg1,
                       (dacsample_t *)static_dac_buffer, DAC_BUFFER_SIZE);
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
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Starting...\r\n");
  chprintf(chp, "System tick freq: %u Hz\n\r", CH_CFG_ST_FREQUENCY);

  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);

  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  generate_dac_buffer(dac_buffer, DAC_BUFFER_SIZE);
  SCB_CleanDCache_by_Addr((uint32_t*)dac_buffer, DAC_BUFFER_SIZE * sizeof(dacsample_t));
  /* Starting DAC1 driver.*/
  dacStart(&DACD1, &dac1cfg1);

  /* Starting GPT6 driver, it is used for triggering the DAC.*/
  gptStart(&GPTD6, &gpt6cfg1);

  /*
  for (uint32_t i = 0; i < sizeof(dac_buffer) / sizeof(dac_buffer[i]); i++) {
    chprintf(chp, "gen: %d | ref: %d\r\n", dac_buffer[i], static_dac_buffer[i]);
  }
  */
  /* Starting a continuous conversion.*/
  dacStartConversion(&DACD1, &dacgrpcfg1,
                     (dacsample_t *)static_dac_buffer, DAC_BUFFER_SIZE);

  float frequency_hz = 440.0f;
  const uint32_t num_samples = DAC_BUFFER_SIZE;
  uint32_t dac_update_rate = (uint32_t)(frequency_hz * num_samples);


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
  }
  return 0;
}
