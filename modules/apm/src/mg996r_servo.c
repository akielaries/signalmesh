#include <math.h>
#include "ch.h"
#include "hal.h"
#include "ccportab.h"
#include "chprintf.h"


#include "portab.h"


#define SERVO_MIN_US 600
#define SERVO_MAX_US 2400

void servo_set_pos(uint16_t us) {
  if (us < SERVO_MIN_US) us = SERVO_MIN_US;
  if (us > SERVO_MAX_US) us = SERVO_MAX_US;
  pwmEnableChannel(&PWMD1, 1, us);
}

void servo_set_angle(float degrees) {
  if (degrees < 0) degrees = 0;
  if (degrees > 180) degrees = 180;
  
  uint16_t us = SERVO_MIN_US + (degrees / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US);
  servo_set_pos(us);
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

  for (uint8_t i = 0; i < 5; i++) {
    chprintf(chp, "moving to 0 deg\r\n");
    servo_set_angle(0);
    chThdSleepMilliseconds(3000);

    chprintf(chp, "moving to 90 deg\r\n");
    servo_set_angle(90);
    chThdSleepMilliseconds(3000);

    chprintf(chp, "moving to 180 deg?\r\n");
    servo_set_angle(180);
    chThdSleepMilliseconds(3000);
  }

  chprintf(chp, "disabling PWM\r\n");
  pwmDisableChannel(&PWMD1, 1);
  pwmStop(&PWMD1);


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
