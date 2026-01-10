#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/driver_api.h"
#include "drivers/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GM009605_DEFAULT_ADDRESS 0x3C

// SSD1306 Commands
#define SSD1306_SET_CONTRAST          0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME 0xA4
#define SSD1306_DISPLAY_ALL_ON        0xA5
#define SSD1306_NORMAL_DISPLAY        0xA6
#define SSD1306_INVERT_DISPLAY        0xA7
#define SSD1306_DISPLAY_OFF           0xAE
#define SSD1306_DISPLAY_ON            0xAF
#define SSD1306_SET_DISPLAY_OFFSET    0xD3
#define SSD1306_SET_COM_PINS          0xDA
#define SSD1306_SET_VCOM_DETECT       0xDB
#define SSD1306_SET_DISPLAY_CLOCK_DIV 0xD5
#define SSD1306_SET_PRECHARGE         0xD9
#define SSD1306_SET_MULTIPLEX         0xA8
#define SSD1306_SET_LOW_COLUMN        0x00
#define SSD1306_SET_HIGH_COLUMN       0x10
#define SSD1306_SET_START_LINE        0x40
#define SSD1306_MEMORY_MODE           0x20
#define SSD1306_COLUMN_ADDR           0x21
#define SSD1306_PAGE_ADDR             0x22
#define SSD1306_COM_SCAN_INC          0xC0
#define SSD1306_COM_SCAN_DEC          0xC8
#define SSD1306_SEGRE_MAP             0xA0
#define SSD1306_CHARGE_PUMP           0x8D
#define SSD1306_EXTERNAL_VCC          0x1
#define SSD1306_SWITCH_CAP_VCC        0x2

// Scrolling Commands
#define SSD1306_ACTIVATE_SCROLL                      0x2F
#define SSD1306_DEACTIVATE_SCROLL                    0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A


#define OLED_WIDTH  128
#define OLED_HEIGHT 64

#define OLED_CTRL_WIDTH 132
#define OLED_VISIBLE_WIDTH 128
#define OLED_X_OFFSET 2

enum oled_ioctl {
  OLED_IOCTL_TEST_PATTERN_HZTL_RAMP,
  OLED_IOCTL_TEST_PATTERN_VERT_RAMP,
  OLED_IOCTL_SET_BRIGHTNESS,
};


typedef struct {
  i2c_bus_t bus;
  bool initialized;
} gm009605_t;


extern const driver_t gm009605_driver;

#ifdef __cplusplus
}
#endif
