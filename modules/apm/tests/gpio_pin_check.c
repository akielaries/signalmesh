// pin self-test: drive each pin high then low and read the actual pin level
// back via the input register (palReadPad reflects the pad even in output
// mode). tells you if the STM32 pin itself is healthy - no meter needed.
//   OK    -> pin drives both levels: fault is the wire/FPGA side
//   FAULT -> pin can't reach a level: pin damaged or externally forced

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

static void pin_selftest(const char *name, ioportid_t port, uint8_t pad) {
  palSetPadMode(port, pad, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(port, pad);
  chThdSleepMilliseconds(2);
  unsigned rb_hi = palReadPad(port, pad);
  palClearPad(port, pad);
  chThdSleepMilliseconds(2);
  unsigned rb_lo = palReadPad(port, pad);
  bsp_printf("%s: drive1 read %u, drive0 read %u -> %s\n",
             name, rb_hi, rb_lo,
             (rb_hi == 1U && rb_lo == 0U) ? "OK" : "FAULT");
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- pin self-test (drive + readback via IDR) ---\n");

  while (true) {
    pin_selftest("PD8 (AD13 ref)", GPIOD, 8);
    pin_selftest("PD9 (AD14)    ", GPIOD, 9);
    pin_selftest("PD0 (AD2)     ", GPIOD, 0);
    pin_selftest("PD1 (AD3 ref) ", GPIOD, 1);
    bsp_printf("---\n");
    chThdSleepMilliseconds(1000);
  }
}
