// barebones pot read via the 74HC4052 mux on the control board.
//
// wiring:
//   STM32 PF8 -> 4052 S0 (pin 10),  PF9 -> 4052 S1 (pin 9)     select bits
//   STM32 PC0 <- 4052 COM A (pin 13),  PC3 <- 4052 COM B (pin 3) analog commons
//   sliders on the 8 channels: X0..X3 = osc1 A/D/S/R, Y0..Y3 = osc2 A/D/S/R
//
// this scans all 4 select values and prints both commons each pass. only one
// slider is wired so far, so watch the one value that sweeps as you slide it.
// channel index = S1*2 + S0, so:
//   ch0 = X0/Y0 (Attack)   ch1 = X1/Y1 (Decay)
//   ch2 = X2/Y2 (Sustain)  ch3 = X3/Y3 (Release)
// COM_A = osc1 bank, COM_B = osc2 bank. (osc2 Release = ch3, COM_B.)

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/adc.h"

// 4052 select lines
#define MUX_SEL_A_LINE PAL_LINE(GPIOF, 8U)
#define MUX_SEL_B_LINE PAL_LINE(GPIOF, 9U)

// index into the POTS group (bsp_adc_config.c order): [0]=PC0=COM A, [1]=PC3=COM B
#define COM_A 0U
#define COM_B 1U

// on the board PF8 landed on S1 (pin 9) and PF9 on S0 (pin 10) - swapped from the
// plan - so PF8 carries bit1 and PF9 carries bit0. channel = S1*2 + S0.
static void mux_select(uint8_t ch) {
  palWriteLine(MUX_SEL_A_LINE, (ch & 2U) ? PAL_HIGH : PAL_LOW);   // PF8 -> S1 (pin 9)
  palWriteLine(MUX_SEL_B_LINE, (ch & 1U) ? PAL_HIGH : PAL_LOW);   // PF9 -> S0 (pin 10)
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_pot_mux: 4052 scan (PF8/PF9 select, PC0/PC3 commons) ---\r\n");

  // select lines as push-pull outputs
  palSetLineMode(MUX_SEL_A_LINE, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(MUX_SEL_B_LINE, PAL_MODE_OUTPUT_PUSHPULL);

  // PC0/PC3 are already configured in the POTS group; just start it (16-bit, continuous)
  adc_init();
  adc_start_continuous(1U << ADC_GROUP_POTS);

  uint16_t s[4];
  while (true) {
    for (uint8_t ch = 0U; ch < 4U; ch++) {
      mux_select(ch);
      // the ADC free-runs, so after switching the mux we let it cycle, then
      // discard one frame (it may straddle the switch) and take a fresh one
      chThdSleepMilliseconds(8);
      adc_get_samples(ADC_GROUP_POTS, s, 4U);   // stale/straddling frame, discard
      chThdSleepMilliseconds(4);
      adc_get_samples(ADC_GROUP_POTS, s, 4U);   // fresh frame for this channel
      bsp_printf("ch%u  COM_A(PC0)=%5u  COM_B(PC3)=%5u\r\n",
                 (unsigned)ch, (unsigned)s[COM_A], (unsigned)s[COM_B]);
    }
    bsp_printf("\r\n");
    chThdSleepMilliseconds(300);
  }
}
