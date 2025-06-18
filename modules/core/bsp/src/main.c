/**
 * @brief BSP main entry point
 */
#include "ch.h"
#include "hal.h"

// declare a symbol that must be defined by the application
extern void app_main(void);

int main(void) {
  halInit();
  chSysInit();

  app_main();  // application takes over from here
}

