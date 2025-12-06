#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "drivers/servo.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/utils/bsp_io.h"

// blinky thread
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetLine(LINE_LED1);
    chThdSleepMilliseconds(100);
    palClearLine(LINE_LED1);
    chThdSleepMilliseconds(100);
  }
}

static THD_WORKING_AREA(waThread2, 128);
static THD_FUNCTION(Thread2, arg) {
  (void)arg;
  chRegSetThreadName("blinker2");
  while (true) {
    palSetLine(LINE_LED3);
    chThdSleepMilliseconds(300);
    palClearLine(LINE_LED3);
    chThdSleepMilliseconds(300);
  }
}


/*
 * Application entry point.
 */
int main(void) {
  bsp_init();
  servo_init();

  bsp_printf("Blinking on boot\n");
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);


  for (uint8_t i = 0; i < 3; i++) {
    bsp_printf("moving to 0 deg\n");
    servo_set_angle(0);
    chThdSleepMilliseconds(500);

    bsp_printf("moving to 180 deg\n");
    servo_set_angle(180);
    chThdSleepMilliseconds(500);

    bsp_printf("moving to 90 deg?\n");
    servo_set_angle(90);
    chThdSleepMilliseconds(500);
  }

  bsp_printf("disabling PWM\n");
  servo_stop();


  while (true) {
    chThdSleepMilliseconds(500);
  }
  return 0;
}
