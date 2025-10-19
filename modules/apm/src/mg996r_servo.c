#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"


static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(100);
    palClearLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(100);

    palSetLine(PORTAB_LINE_LED2);
    chThdSleepMilliseconds(100);
    palClearLine(PORTAB_LINE_LED2);
    chThdSleepMilliseconds(100);

    palSetLine(PORTAB_LINE_LED3);
    chThdSleepMilliseconds(100);
    palClearLine(PORTAB_LINE_LED3);
    chThdSleepMilliseconds(100);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /* Board-dependent GPIO setup code.*/
  portab_setup();
  chprintf(chp, "\r\n...Starting...\r\n\r\n");
  chprintf(chp, "System tick freq: %u Hz\r\n", CH_CFG_ST_FREQUENCY);

  chprintf(chp, "Blinking on boot\r\n");
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  while (true) {
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      gptStopTimer(&GPTD6);
      dacStopConversion(&DACD1);
    }
    chThdSleepMilliseconds(500);
    //chprintf(chp, "arggg....\r\n");
  }
  return 0;
}
