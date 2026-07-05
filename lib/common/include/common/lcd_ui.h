#ifndef COMMON_LCD_UI_H
#define COMMON_LCD_UI_H

#include <stddef.h>
#include <stdint.h>

// 20x04 i2c character-lcd boot-log / status helpers over the lcd2004 driver

#define LCD_COLS 20U
#define LCD_ROWS 4U

// compile-time length guards - only valid for string LITERALS (sizeof is a
// constant there). use the plain lcd_* functions for runtime-formatted strings
// (bound those with a char[LCD_COLS + 1] buffer + chsnprintf instead).
#define LCD_LINE(row, lit)                                                     \
  do {                                                                         \
    _Static_assert(sizeof(lit) - 1U <= LCD_COLS, "LCD line too long: " lit);   \
    lcd_line((row), (lit));                                                    \
  } while (0)

#define LCD_MSG(lit)                                                           \
  do {                                                                         \
    _Static_assert(sizeof(lit) - 1U <= LCD_COLS * LCD_ROWS,                     \
                   "LCD msg too long: " lit);                                  \
    lcd_wrap((lit));                                                           \
  } while (0)

#define LCD_LOG(lit)                                                           \
  do {                                                                         \
    _Static_assert(sizeof(lit) - 1U <= LCD_COLS, "LCD log too long: " lit);    \
    lcd_log((lit));                                                            \
  } while (0)

#ifdef __cplusplus
extern "C" {
#endif

// find + clear the lcd2004 device. returns 0 if present, -1 if not (then every
// call below is a safe no-op, so boot code never needs to null-check the lcd)
int lcd_ui_init(void);
void lcd_clear(void);

// write one row (0..3), truncated and space-padded to 20 columns
void lcd_line(uint8_t row, const char *s);

// write a string across the rows, 20 chars per row
void lcd_wrap(const char *s);

// push a line into the rolling 4-row log (scrolls up, newest on the bottom row)
void lcd_log(const char *s);

#ifdef __cplusplus
}
#endif

#endif // COMMON_LCD_UI_H
