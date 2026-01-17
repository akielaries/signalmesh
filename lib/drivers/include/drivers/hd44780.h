#pragma once

#include "drivers/i2c.h"
#include "drivers/driver_api.h"

#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x04
#define LCD_CMD_DISPLAY_CONTROL 0x08
#define LCD_CMD_CURSOR_SHIFT    0x10
#define LCD_CMD_FUNCTION_SET    0x20
#define LCD_CMD_SET_CGRAM_ADDR  0x40
#define LCD_CMD_SET_DDRAM_ADDR  0x80

#define LCD_ENTRY_RIGHT           0x00
#define LCD_ENTRY_LEFT            0x02
#define LCD_ENTRY_SHIFT_INCREMENT 0x01
#define LCD_ENTRY_SHIFT_DECREMENT 0x00

#define LCD_DISPLAY_ON  0x04
#define LCD_DISPLAY_OFF 0x00
#define LCD_CURSOR_ON   0x02
#define LCD_CURSOR_OFF  0x00
#define LCD_BLINK_ON    0x01
#define LCD_BLINK_OFF   0x00

#define LCD_8BIT_MODE 0x10
#define LCD_4BIT_MODE 0x00
#define LCD_2LINE     0x08
#define LCD_1LINE     0x00
#define LCD_5x10_DOTS 0x04
#define LCD_5x8_DOTS  0x00

#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

typedef struct {
  uint8_t addr;
  uint8_t backlight;
} lcd2004_config_t;

typedef struct {
  const lcd2004_config_t *config;
  uint8_t display_function;
  uint8_t display_control;
  uint8_t display_mode;
} lcd2004_t;

extern const driver_t lcd2004_driver;

int lcd2004_init(device_t *dev);
int32_t
lcd2004_draw(device_t *dev, uint8_t page, uint8_t col, const uint8_t *buffer, size_t buffer_size);
int lcd2004_write(device_t *dev, uint32_t offset, const void *buf, size_t count);
int lcd2004_clear(device_t *dev);
int lcd2004_ioctl(device_t *dev, uint32_t cmd, void *arg);

typedef struct {
  uint8_t col;
  uint8_t row;
} lcd2004_cursor_pos_t;


#define LCD_IOCTL_SET_CURSOR 1
