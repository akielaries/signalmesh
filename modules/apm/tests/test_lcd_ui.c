// exercises the lcd_ui boot-log helpers: rolling log (LCD_LOG + a formatted
// runtime line), fixed-row status board (LCD_LINE), and a wrapped message
// (LCD_MSG). the _Static_assert guards fire at compile time if any literal is
// too long - try lengthening one to see it.

#include <stdio.h> // snprintf

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "common/lcd_ui.h"

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_lcd_ui ---\r\n");

  if (lcd_ui_init() != 0) {
    bsp_printf("lcd2004 not found\r\n");
    while (true) {
      chThdSleepMilliseconds(1000);
    }
  }

  LCD_LOG("ACM boot...");
  chThdSleepMilliseconds(500);
  LCD_LOG("5V rail      OK");
  chThdSleepMilliseconds(500);
  LCD_LOG("3V3 rail     OK");
  chThdSleepMilliseconds(500);
  LCD_LOG("FMC bus      OK");
  chThdSleepMilliseconds(500);
  LCD_LOG("FPGA cfg     OK");
  chThdSleepMilliseconds(500);

  char buf[LCD_COLS + 1];
  snprintf(buf, sizeof(buf), "magic 0x%04X OK", 0xACE1U);
  lcd_log(buf);
  chThdSleepMilliseconds(500);
  snprintf(buf, sizeof(buf), "id 0x%04X  20K", 0x2018U);
  lcd_log(buf);
  chThdSleepMilliseconds(1800);

  // 2) fixed-row status board
  lcd_clear();
  LCD_LINE(0, "=====  ACM  =====");
  LCD_LINE(1, "FPGA 0x2018 (20K)");
  LCD_LINE(2, "5V 4.98  3V3 3.31");
  LCD_LINE(3, ">>>>  READY  <<<<");
  chThdSleepMilliseconds(2500);

  lcd_clear();
  LCD_MSG("ACM Synth booted - "
          "turn a slider or play "
          "a note to make sound.");
  LCD_MSG("ACM FPGA booted...");
  chThdSleepMilliseconds(2500);

  // loop the rolling log so you can watch it scroll
  lcd_clear();
  uint32_t n = 0U;
  for (;;) {
    snprintf(buf, sizeof(buf), "tick %lu", (unsigned long)n++);
    lcd_log(buf);
    chThdSleepMilliseconds(1000);
  }
}
