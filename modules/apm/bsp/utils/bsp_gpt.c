#include "bsp/utils/bsp_gpt.h"
#include "hal.h" // For gptStart, etc.

void bsp_gpt_init(bsp_gpt_driver_t *gptd) {
    if (gptd == NULL || gptd->gptd == NULL || gptd->config == NULL) {
        return;
    }
    gptStart(gptd->gptd, gptd->config);
}

void bsp_gpt_start_continuous(bsp_gpt_driver_t *gptd, uint32_t interval) {
    if (gptd == NULL || gptd->gptd == NULL) {
        return;
    }
    gptStartContinuous(gptd->gptd, interval);
}

void bsp_gpt_stop(bsp_gpt_driver_t *gptd) {
    if (gptd == NULL || gptd->gptd == NULL) {
        return;
    }
    gptStopTimer(gptd->gptd);
}
