#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"


// blinky thread
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

// Example function
void servo_set_pos(uint16_t us) {
  if (us < 1000) us = 1000;
  if (us > 2000) us = 2000;
  // 1mhz base = microsecond width?
  pwmEnableChannel(&PWMD3, 3, PWM_FRACTION_TO_WIDTH(&PWMD3, 1000000, us));
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

  // start pwm driver
  pwmStart(&PWMD3, &portabpwmgrpcfg1);

  chprintf(chp, "moving to 0 deg\r\n");
  servo_set_pos(1000); // 0°
  chThdSleepMilliseconds(10000);

  chprintf(chp, "moving to 90 deg\r\n");
  servo_set_pos(1500); // 90°
  chThdSleepMilliseconds(10000);

  chprintf(chp, "moving to 180 deg?\r\n");
  servo_set_pos(2000); // 180°
  chThdSleepMilliseconds(10000);


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
