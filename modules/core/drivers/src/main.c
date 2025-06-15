/**
 * signalmesh 2025
 *
 * @file
 * @brief testing some H755 code
 * @author Akiel Aries
 */

#include "ch.h"
#include "chprintf.h"

#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"



static SerialConfig const uart5_cfg = {
  //.speed = 115200,
  .speed = 1000000,
  .cr1 = 0,
  .cr2 = USART_CR2_STOP1_BITS,
  .cr3 = 0,
};

/*
 * This is a periodic thread that does absolutely nothing except flashing
 * a LED.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOC, GPIOC_PIN0, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOC, GPIOC_PIN3, PAL_STM32_MODE_OUTPUT);

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    // set
    palSetLine(LINE_LED1);
    chThdSleepMilliseconds(200);
    palSetLine(LINE_LED2);
    chThdSleepMilliseconds(200);
    palSetLine(LINE_LED3);
    chThdSleepMilliseconds(200);

    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(200);
    palSetPad(GPIOC, GPIOC_PIN0);
    chThdSleepMilliseconds(200);
    palSetPad(GPIOC, GPIOC_PIN3);
    chThdSleepMilliseconds(200);

    // clear
    palClearLine(LINE_LED1);
    chThdSleepMilliseconds(200);
    palClearLine(LINE_LED2);
    chThdSleepMilliseconds(200);
    palClearLine(LINE_LED3);
    chThdSleepMilliseconds(200);

    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(200);
    palClearPad(GPIOC, GPIOC_PIN0);
    chThdSleepMilliseconds(200);
    palClearPad(GPIOC, GPIOC_PIN3);
    chThdSleepMilliseconds(200);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8)); // TX
  palSetPadMode(GPIOD, GPIOD_PIN2,  PAL_MODE_ALTERNATE(8)); // RX
  palSetPadMode(GPIOG, GPIOG_PIN3, PAL_MODE_INPUT); // ext push button
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT); // blue LED?

  sdStart(&SD3, NULL);
  sdStart(&SD5, &uart5_cfg);
  BaseSequentialStream* chp = (BaseSequentialStream*) &SD5;

  chprintf(chp, "start of program...\r\n");
  // flash some leds to signal the start
  for (uint8_t i = 0; i < 10; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(50);
  }

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  int counter = 0;
  while (1) {
    if (palReadLine(LINE_BUTTON) || palReadPad(GPIOG, GPIOG_PIN3)) {
      chprintf(chp, "exiting!!!\r\n\r\n");

      test_execute((BaseSequentialStream *)&SD3, &rt_test_suite);
      test_execute((BaseSequentialStream *)&SD3, &oslib_test_suite);
    }
    chThdSleepMilliseconds(1000);
    chprintf(chp, "printing counter: %d\r\n", counter);
    counter++;
  }
}
