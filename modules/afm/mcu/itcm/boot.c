#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

#include "hw.h"

#include "sysinfo_regs.h"
#include "gpio_regs.h"

#include <stdint.h>


/* ========================================================= */

// IDLE THREAD! this should ideally just be main() i think but idk
THREAD_STACK(idle, 256);
THREAD_FUNCTION(idle_fn, arg) {
  while (1) {
    __WFI();
  }
}

THREAD_STACK(uptime, 512);
THREAD_FUNCTION(uptime_fn, arg) {
  while (1) {
    dbg_printf("uptime: %ds\r\n", system_time_ms / 1000);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(blink1_thd, 512);
THREAD_FUNCTION(blink1_fn, arg) {
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system and the threads
 */
int main(void) {
  hw_init();

  // start the scheduler/kernel
  dbg_printf("initializing kernel...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");

  // idle thread with lowest priority
  //mkthd_static(idle, idle_fn, sizeof(idle), PRIO_LOW, NULL);
  mkthd_static(uptime, uptime_fn, sizeof(uptime), PRIO_NORMAL, NULL);

  mkthd_static(blink1_thd, blink1_fn, sizeof(blink1_thd), PRIO_NORMAL, NULL);
  mkthd_static(blink2_thd, blink2_fn, sizeof(blink2_thd), PRIO_NORMAL, NULL);

  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
