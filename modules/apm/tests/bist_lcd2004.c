#include "ch.h"
#include "hal.h"

#include "bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/lcd2004.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include <string.h> // For strlen

// ChibiOS specific printf for embedded systems
#include <stdio.h> // Temporarily for snprintf, will be replaced by chsnprintf

#define LCD_IOCTL_SET_CURSOR 1

void test_simple_string(device_t* lcd) {
    bsp_printf("--- Test: Simple String ---\r\n");
    lcd->driver->clear(lcd);
    lcd->driver->write(lcd, 0, "Hello, World!", strlen("Hello, World!"));
    chThdSleepMilliseconds(2000);
}

void test_multi_line(device_t* lcd) {
    bsp_printf("--- Test: Multi-line String ---\r\n");
    lcd->driver->clear(lcd);
    lcd2004_cursor_pos_t pos0 = { .col = 0, .row = 0 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos0);
    lcd->driver->write(lcd, 0, "Line 1", strlen("Line 1"));
    lcd2004_cursor_pos_t pos1 = { .col = 0, .row = 1 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos1);
    lcd->driver->write(lcd, 0, "Line 2", strlen("Line 2"));
    lcd2004_cursor_pos_t pos2 = { .col = 0, .row = 2 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos2);
    lcd->driver->write(lcd, 0, "Line 3", strlen("Line 3"));
    lcd2004_cursor_pos_t pos3 = { .col = 0, .row = 3 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos3);
    lcd->driver->write(lcd, 0, "Line 4", strlen("Line 4"));
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

    lcd2004_cursor_pos_t pos = { .col = 0, .row = 0 };
    lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
    lcd->driver->write(lcd, 0, "Progress:", strlen("Progress:"));

    for (int p = 0; p <= 100; p += 5) {
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
        pos.col = 10;
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
        char percent_str[5];
        chsnprintf(percent_str, sizeof(percent_str), "%3d%%", p); // Using chsnprintf
        lcd->driver->write(lcd, 0, percent_str, strlen(percent_str));

        pos.col = 15;
        lcd->driver->ioctl(lcd, LCD_IOCTL_SET_CURSOR, &pos);
        for (int i = 0; i < 4; i++) { // 4 characters for the bar
            uint8_t char_to_display = 0;
            if (p >= (i + 1) * 25) { // Full segment
                char_to_display = 5;
            } else if (p > i * 25) { // Partial segment
                char_to_display = (uint8_t)((p - (i * 25)) / 5) + 1;
            }
            lcd->driver->write(lcd, 0, (const char*)&char_to_display, 1);
        }
        chThdSleepMilliseconds(100);
    }
    chThdSleepMilliseconds(2000);
}

int main(void) {
    bsp_init();

    bsp_printf("\n--- Starting LCD20x04 BIST ---\r\n");

    device_t* lcd = find_device("lcd2004");
    if (lcd) {
        test_simple_string(lcd);
        test_multi_line(lcd);
        test_custom_chars(lcd);
    }

    bsp_printf("--- BIST Complete ---\r\n");

    while (1) {
        chThdSleepMilliseconds(1000);
    }
}
