#include "ch.h"
#include "hal.h"

#include "bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/hd44780_i2c.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include <string.h>

void test_simple_string(device_t* lcd) {
    bsp_printf("--- Test: Simple String ---\r\n");
    lcd->driver->clear(lcd);
    lcd->driver->write(lcd, 0, "Hello, World!", strlen("Hello, World!"));
    chThdSleepMilliseconds(2000);
}

void test_multi_line(device_t* lcd) {
    bsp_printf("--- Test: Multi-line String ---\r\n");
    lcd->driver->clear(lcd);
    hd44780_i2c_cursor_pos_t pos = { .col = 0, .row = 0 };
    for (uint8_t i = 0; i < ((hd44780_i2c_t*)lcd->priv)->config->rows; i++) {
        pos.row = i;
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
        char line_str[16];
        chsnprintf(line_str, sizeof(line_str), "Line %d", i + 1);
        lcd->driver->write(lcd, 0, line_str, strlen(line_str));
    }
    chThdSleepMilliseconds(2000);
}

void test_custom_chars(device_t* lcd) {
    bsp_printf("--- Test: Custom Characters (Progress Bar) ---\r\n");
    lcd->driver->clear(lcd);

    // Define custom characters for a progress bar
    uint8_t customChar[8][8] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 0: Space
        { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 }, // 1: 1/5 filled
        { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 }, // 2: 2/5 filled
        { 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C }, // 3: 3/5 filled
        { 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E }, // 4: 4/5 filled
        { 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F }, // 5: Full
    };

    // Send custom characters to LCD CGRAM
    for (int i = 0; i < 6; i++) {
        lcd->driver->draw(lcd, 0xFF, i, customChar[i], 8);
    }

    hd44780_i2c_cursor_pos_t pos = { .col = 0, .row = 0 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
    lcd->driver->write(lcd, 0, "Progress:", strlen("Progress:"));

    for (int p = 0; p <= 100; p += 5) {
        pos.col = 10;
        pos.row = 0;
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
        char percent_str[5];
        chsnprintf(percent_str, sizeof(percent_str), "%3d%%", p);
        lcd->driver->write(lcd, 0, percent_str, strlen(percent_str));

        pos.col = 0;
        pos.row = 1;
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);

        for (int i = 0; i < ((hd44780_i2c_t*)lcd->priv)->config->cols; i++) {
            uint8_t char_to_display = 0;
            if (p/5 > i) {
                char_to_display = 5;
            }
            lcd->driver->write(lcd, 0, (const char*)&char_to_display, 1);
        }
        chThdSleepMilliseconds(100);
    }
    chThdSleepMilliseconds(2000);
}

void run_tests(device_t* lcd) {
    if (lcd) {
        bsp_printf("\n--- Testing %s ---\r\n", lcd->name);
        test_simple_string(lcd);
        test_multi_line(lcd);
        test_custom_chars(lcd);
    } else {
        bsp_printf("\n--- LCD device not found ---\r\n");
    }
}

int main(void) {
    bsp_init();

    bsp_printf("\n--- Starting HD44780 I2C BIST ---\r\n");

    device_t* lcd2004 = find_device("hd44780_2004");
    run_tests(lcd2004);

    device_t* lcd1602 = find_device("hd44780_1602");
    run_tests(lcd1602);

    bsp_printf("--- BIST Complete ---\r\n");

    while (1) {
        chThdSleepMilliseconds(1000);
    }
}
