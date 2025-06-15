#include "ch.h"
#include "hal.h"
#include "chprintf.h"


#define LINE_EXT_BUTTON PAL_LINE(GPIOE, 3)
#define LINE_TRIGGER_OUT PAL_LINE(GPIOE, 6)


static BaseSequentialStream *chp    = (BaseSequentialStream *)&SD5;
static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};


static virtual_timer_t vt;
static binary_semaphore_t buttonSem;


// callback for button press
static void button_cb(void *arg);
// virtual timer callback
static void vt_cb(virtual_timer_t *vtp, void *p);


/* Callback of the virtual timer. */
static void vt_cb(virtual_timer_t *vtp, void *p) {
  (void)vtp;
  (void)p;
  chSysLockFromISR();
  /* Enabling the event and associating the callback. */
  palEnableLineEventI(LINE_EXT_BUTTON, PAL_EVENT_MODE_RISING_EDGE);
  palSetLineCallbackI(LINE_EXT_BUTTON, button_cb, NULL);
  chSysUnlockFromISR();
}

static void button_cb(void *arg) {
  (void)arg;
  palToggleLine(LINE_LED1);
  chSysLockFromISR();
  chBSemSignalI(&buttonSem);  // Notify main thread
  /* Disabling the event on the line and setting a timer to
     re-enable it. */
  palDisableLineEventI(LINE_EXT_BUTTON);
  /* Arming the VT timer to re-enable the event in 50ms. */
  chVTResetI(&vt);
  chVTDoSetI(&vt, TIME_MS2I(50), vt_cb, NULL);
  chSysUnlockFromISR();
}

int main(void) {
  halInit();
  chSysInit();
  chVTObjectInit(&vt);
  chBSemObjectInit(&buttonSem, true);  // Initialize semaphore

  // init UART
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
  // init LEDs
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);

  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Starting up...\r\n");
  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  palSetLineMode(LINE_EXT_BUTTON, PAL_MODE_INPUT_PULLUP);
  palSetLineMode(LINE_TRIGGER_OUT, PAL_MODE_OUTPUT_PUSHPULL);

  /* Enabling the event and associating the callback. */
  palEnableLineEvent(LINE_EXT_BUTTON, PAL_EVENT_MODE_RISING_EDGE);
  palSetLineCallback(LINE_EXT_BUTTON, button_cb, NULL);


  chprintf(chp, "main loop...\r\n");

  while (true) {
    // Reset semaphore to ensure we're not reacting to old signals
    chBSemReset(&buttonSem, TRUE);

    // Simulate external interrupt by toggling PE6
    palClearLine(LINE_TRIGGER_OUT);
    chThdSleepMilliseconds(10);
    palSetLine(LINE_TRIGGER_OUT); // Rising edge
    chThdSleepMilliseconds(10);
    palClearLine(LINE_TRIGGER_OUT); // Back to low

    // Wait for interrupt to fire
    if (chBSemWaitTimeout(&buttonSem, TIME_MS2I(100)) == MSG_OK) {
      chprintf(chp, "interrupt fired...\r\n");
      palSetLine(LINE_LED3);
    } else {
      chprintf(chp, "no interrupt detected\r\n");
    }

    palSetLine(LINE_LED2);
    chThdSleepMilliseconds(200);
    palClearLine(LINE_LED2);
    chThdSleepMilliseconds(200);
  }

}
