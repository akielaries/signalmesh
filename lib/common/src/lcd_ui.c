#include "common/lcd_ui.h"

#include <string.h>

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/lcd2004.h"

static device_t *g_lcd;
static char g_log[LCD_ROWS][LCD_COLS + 1U]; // rolling log rows, top -> bottom

// write s at column 0 of row, truncated to LCD_COLS and space-padded so any
// stale characters left on the row are cleared
static void put_row(uint8_t row, const char *s) {
  if (g_lcd == NULL) {
    return;
  }
  char line[LCD_COLS + 1U];
  size_t n = strlen(s);
  if (n > LCD_COLS) {
    n = LCD_COLS;
  }
  memcpy(line, s, n);
  memset(line + n, ' ', LCD_COLS - n);
  line[LCD_COLS] = '\0'; // the driver writes until '\0' (ignores count), so terminate
  lcd2004_cursor_pos_t pos = {.col = 0U, .row = row};
  g_lcd->driver->ioctl(g_lcd, LCD_IOCTL_SET_CURSOR, &pos);
  g_lcd->driver->write(g_lcd, 0U, line, LCD_COLS);
}

int lcd_ui_init(void) {
  memset(g_log, 0, sizeof(g_log));
  g_lcd = find_device("lcd2004");
  if (g_lcd == NULL) {
    return -1;
  }
  // init handled centrally by init_devices() in bsp_init; just clear here
  g_lcd->driver->clear(g_lcd);
  return 0;
}

void lcd_clear(void) {
  memset(g_log, 0, sizeof(g_log));
  if (g_lcd != NULL) {
    g_lcd->driver->clear(g_lcd);
  }
}

void lcd_line(uint8_t row, const char *s) {
  if (row < LCD_ROWS) {
    put_row(row, s);
  }
}

void lcd_wrap(const char *s) {
  size_t len = strlen(s);
  for (uint8_t r = 0U; r < LCD_ROWS; r++) {
    size_t off = (size_t)r * LCD_COLS;
    if (off >= len) {
      put_row(r, "");
      continue;
    }
    size_t n = len - off;
    if (n > LCD_COLS) {
      n = LCD_COLS;
    }
    char seg[LCD_COLS + 1U];
    memcpy(seg, s + off, n);
    seg[n] = '\0';
    put_row(r, seg);
  }
}

void lcd_log(const char *s) {
  // scroll the rows up, drop the oldest, put the new line on the bottom
  for (uint8_t r = 0U; r < LCD_ROWS - 1U; r++) {
    memcpy(g_log[r], g_log[r + 1U], LCD_COLS + 1U);
  }
  size_t n = strlen(s);
  if (n > LCD_COLS) {
    n = LCD_COLS;
  }
  memcpy(g_log[LCD_ROWS - 1U], s, n);
  g_log[LCD_ROWS - 1U][n] = '\0';
  for (uint8_t r = 0U; r < LCD_ROWS; r++) {
    put_row(r, g_log[r]);
  }
}
