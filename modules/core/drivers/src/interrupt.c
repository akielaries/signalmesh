#include "ch.h"
#include "hal.h"
#include "chprintf.h"

static BaseSequentialStream *chp    = (BaseSequentialStream *)&SD5;
static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

#define LINE_PG0        PAL_LINE(GPIOG, 0)
#define LINE_EXT_BUTTON PAL_LINE(GPIOG, 0)


int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetLineMode(LINE_LED_GREEN, PAL_MODE_OUTPUT_PUSHPULL);

  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Starting up...\r\n");

  /* Configure PG0 as input with pull-up (more reliable for buttons) */
  palSetLineMode(LINE_EXT_BUTTON, PAL_MODE_INPUT_PULLUP);
  /* Enabling the event on the Line for a Rising edge. */
  palEnableLineEvent(LINE_EXT_BUTTON, PAL_EVENT_MODE_RISING_EDGE);

  chprintf(chp, "Press button to trigger interrupt\r\n");

  chprintf(chp, "main loop...\r\n");
  while (true) {
    chprintf(chp, "waiting for interrupt...\r\n");

    /* Waiting for the event to happen. */
    palWaitLineTimeout(LINE_EXT_BUTTON, TIME_INFINITE);
    chprintf(chp, "interrupt fired...");
    palSetLine(LINE_LED_GREEN);

    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
  }
}
