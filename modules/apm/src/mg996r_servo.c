#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"

void servo_set_pos(uint16_t us) {
  if (us < 1000) us = 1000;
  if (us > 2000) us = 2000;
  pwmEnableChannel(&PWMD1, 1, us);
}
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

  // start pwm driver
  pwmStart(&PWMD1, &portabpwmgrpcfg1);
  chprintf(chp, "starting PWM channel 1 @%dkHz / %dmHz\r\n",
           portabpwmgrpcfg1.frequency / 1000,
           portabpwmgrpcfg1.frequency / 1000 / 1000);

  chprintf(chp, "moving to 0 deg\r\n");
  servo_set_pos(1000); // 0°
  chThdSleepMilliseconds(3000);

  chprintf(chp, "moving to 90 deg\r\n");
  servo_set_pos(1500); // 90°
  chThdSleepMilliseconds(3000);

  chprintf(chp, "moving to 180 deg?\r\n");
  servo_set_pos(2000); // 180°
  chThdSleepMilliseconds(3000);
  servo_set_pos(2500); // 180°
  chThdSleepMilliseconds(3000);

/*
  // 50% duty cycle
  pwmEnableChannel(&PWMD1, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD1, 5000));
  chThdSleepMilliseconds(3000);
  chprintf(chp, "done sleeping\r\n");
*/
  //pwmEnablePeriodicNotification(&PWMD1);

  // It enables the periodic callback at the end of pulse
  //pwmEnableChannelNotification(&PWMD1,0);

  while (true) {
    chThdSleepMilliseconds(500);
    //chprintf(chp, "arggg....\r\n");
  }
  return 0;
}
