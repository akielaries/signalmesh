#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

#include "hw.h"

#include <stdint.h>


// dummy generator
static uint32_t lcg_state = 12345;

static uint32_t lcg_rand(void) {
  lcg_state = lcg_state * 1664525 + 1013904223;
  return lcg_state;
}

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
    dbg_printf("pin0\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    dbg_printf("pin1\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(fast_thd, 256);
THREAD_FUNCTION(fast_fn, arg) {
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_2);
    //thread_sleep_ms(2);
  }
}

// basic thread that just yields
// some CPU heavy task that will spit out the result every few seconds or
// something related
THREAD_STACK(compute_thd, 512);
THREAD_FUNCTION(compute_fn, arg) {
  while (1) {
    uint32_t iters = 500000 + (lcg_rand() % 5000000);
    uint32_t result = 0;

    uint32_t t_start = system_time_ms;

    for (uint32_t i = 0; i < iters; i++) {
      result += i * i;
      if ((i % 1000) == 0)
        thread_yield(); // could remove this manual yield
    }

    uint32_t elapsed = system_time_ms - t_start;
    dbg_printf("compute result: %u, took %d ms (%d iters)\r\n", result, elapsed, iters);
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
  mkthd_static(idle, idle_fn, sizeof(idle), PRIO_LOW, NULL);
  mkthd_static(uptime, uptime_fn, sizeof(uptime), PRIO_NORMAL, NULL);

  mkthd_static(blink1_thd, blink1_fn, sizeof(blink1_thd), PRIO_NORMAL, NULL);
  mkthd_static(blink2_thd, blink2_fn, sizeof(blink2_thd), PRIO_NORMAL, NULL);

  mkthd_static(fast_thd, fast_fn, sizeof(fast_thd), PRIO_LOW, NULL);
  mkthd_static(compute_thd, compute_fn, sizeof(compute_thd), PRIO_LOW, NULL);


  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
