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

#define LINE_EXT_BUTTON PAL_LINE(GPIOE, 3)


// callback for button press
static void button_cb(void *arg);
// virtual timer callback
static void vt_cb(virtual_timer_t *vtp, void *p);

/* Virtual timer. */
static virtual_timer_t vt;
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

static binary_semaphore_t buttonSem;
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
  /* Enabling the event and associating the callback. */
  palEnableLineEvent(LINE_EXT_BUTTON, PAL_EVENT_MODE_RISING_EDGE);
  palSetLineCallback(LINE_EXT_BUTTON, button_cb, NULL);

  chprintf(chp, "Press button to trigger interrupt\r\n");

  chprintf(chp, "main loop...\r\n");


  while (true) {
    if (chBSemWaitTimeout(&buttonSem, TIME_INFINITE) == MSG_OK) {
      chprintf(chp, "Button pressed!\r\n");
      palSetLine(LINE_LED3);
    }

    palSetLine(LINE_LED2);
    chThdSleepMilliseconds(200);
    palClearLine(LINE_LED2);
    chThdSleepMilliseconds(200);
  }
}
