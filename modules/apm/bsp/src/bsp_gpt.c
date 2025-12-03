#include "hal.h"
#include "bsp_gpt.h"


const GPTConfig apm_gpt_trigger = {
    .frequency = 20000U,
    .callback  = NULL,
    .cr2       = TIM_CR2_MMS_1,  /* MMS = 010 = TRGO on Update Event */
    .dier      = 0U,
}


int apm_bsp_gpt_init(void) {
  // what to put here? gptStart?

  return 0;
}
