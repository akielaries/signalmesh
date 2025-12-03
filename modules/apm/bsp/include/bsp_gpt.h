#pragma once

#include "hal.h"



/** @brief general purpose timer configuration for   */
extern const GPTConfig apm_gpt_trigger;


#ifdef __cplusplus
extern "C" {
#endif

void apm_bsp_gpt_init(void);

#ifdef __cplusplus
}
#endif
