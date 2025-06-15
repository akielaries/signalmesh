#include "ch.h"
#include "hal.h"
#include "chprintf.h"

static BaseSequentialStream *chp = (BaseSequentialStream *)&SD5;
static const SerialConfig uart5_cfg = {
    .speed = 1000000,
    .cr1 = 0,
    .cr2 = USART_CR2_STOP1_BITS,
    .cr3 = 0,
};

#define LINE_PG0 PAL_LINE(GPIOG, 0)
#define LINE_EXT_BUTTON             PAL_LINE(GPIOG, 0)

/* Callback associated to the raising edge of the button line. */
static void button_cb(void *arg) {
  (void)arg;
  chprintf(chp, "PG0 Interrupt!\r\n");
  palToggleLine(LINE_LED_GREEN);
  palSetLine(LINE_LED1);

}

int main(void) {
  halInit();
  chSysInit();
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));

  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "starting up...\r\n");

  //palSetLineMode(LINE_PG0, PAL_MODE_INPUT_PULLDOWN);
  palSetLineMode(LINE_EXT_BUTTON, PAL_MODE_INPUT);

  // Enable event on both edges (rising and falling)
  //palLineEnableEvent(LINE_PG0, PAL_EVENT_MODE_BOTH_EDGES, pg0_callback);
  palEnableLineEvent(LINE_EXT_BUTTON, PAL_EVENT_MODE_RISING_EDGE);
  /* Associating a callback to the Line. */
  palSetLineCallback(LINE_EXT_BUTTON, button_cb, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}

