#include "hal.h"
#include "ch.h"
#include "bsp/bsp.h"
#include "bsp/configs/bsp_uart_config.h" // For bsp_debug_stream and bsp_debug_uart_config
#include "bsp/utils/bsp_gpt.h"
#include "chprintf.h" // For chprintf

void bsp_init(void) {
    halInit();
    chSysInit();

    // Initialize the debug UART
    // These come from the Port <port #> alternate functions
    palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
    palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
    sdStart(bsp_debug_uart_driver, &bsp_debug_uart_config);

    chprintf(bsp_debug_stream, "booting...");
    for (uint8_t i = 0; i < 4; i++) {
        chprintf(bsp_debug_stream, ".");
        chThdSleepMilliseconds(500);
    }
    chprintf(bsp_debug_stream, "\r\n");

    /* ADC inputs.*/
    palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG); // PB1, ADC channel 5
    palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG); // PA3, ADC channel 15
    palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG); // PC0, ADC channel 10
    palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG); // PC3, ADC channel 13

    // PWM on PE11, timer 1 channel 2?
    palSetPadMode(GPIOE, 11, PAL_MODE_ALTERNATE(1));

    // Initialize GPT (from bsp_gpt.c)
    // bsp_gpt_init() should already be called by peripheral drivers
    // that rely on it, e.g. ADC, PWM.
    // For example, if ADC uses a GPT trigger, then adc_init() would call bsp_gpt_init()
    // It's not called directly here to avoid double initialization or conflicts.

    // Initialize other peripherals (ADC, PWM) through their respective BSP init functions
    // adc_init(); // To be created if not already present
    // pwm_init(); // To be created if not already present
}
