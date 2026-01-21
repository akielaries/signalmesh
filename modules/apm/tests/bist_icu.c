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

/*
 * This file is a BIST (Built-In Self Test) for the ICU driver.
 * It is based on the ICU test example from ChibiOS.
 * It is intended to be used with the APM module.
 *
 * It measures a signal on PA0 and prints the frequency.
 */

#include "ch.h"
#include "hal.h"
#include "bsp.h"
#include "bsp/utils/bsp_io.h" // For bsp_printf


// These variables will be updated by the ICU callbacks.
volatile icucnt_t last_width, last_period;

// Callback for ICU width capture.
static void icuwidthcb(ICUDriver *icup) {
  last_width = icuGetWidthX(icup);
}

// Callback for ICU period capture.
static void icuperiodcb(ICUDriver *icup) {
  last_period = icuGetPeriodX(icup);
}

// ICU configuration for this test.
// We are using Timer 5, Channel 1, which corresponds to PA0 on the STM32H7.
// The alternate function for TIM5_CH1 on PA0 is AF2.
// The ICU clock is set to 1MHz for higher resolution.
static ICUConfig icucfg = {
  .mode         = ICU_INPUT_ACTIVE_HIGH,
  .frequency    = 10000000, // 1MHz ICU clock frequency.
  .width_cb     = icuwidthcb,
  .period_cb    = icuperiodcb,
  .overflow_cb  = NULL,
  .channel      = ICU_CHANNEL_1,
  .dier         = 0U,
  .arr          = 0xFFFFFFFFU
};


/*
 * @brief   Test for the ICU driver.
 * @details This function configures PA0 as an ICU input, measures the
 *          width and period of the signal, and prints the frequency.
 */
int main(void) {
  bsp_init();
    /*
    * Initializes the ICU driver 5.
    * PA0 is the ICU input.
    * On STM32H755, PA0 can be connected to TIM2_CH1(AF1) or TIM5_CH1(AF2).
    * From mcuconf.h, we see TIM5 is enabled, so we use ICUD5.
    */
    palSetPadMode(GPIOA, 0, PAL_MODE_ALTERNATE(2));
    icuStart(&ICUD5, &icucfg);
    icuStartCapture(&ICUD5);
    icuEnableNotifications(&ICUD5);

    while (true) {
        float frequency = 0.0f;
        if (last_period > 0) {
            frequency = (float)icucfg.frequency / (float)last_period;
        }

        bsp_printf("Width: %u, Period: %u, Frequency: %.2f Hz (%.2f mHz)\n",
                    last_width, last_period, frequency,
                    frequency / 1000000);

        chThdSleepMilliseconds(500);
    }
}
