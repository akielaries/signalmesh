#include "drivers/lcd2004.h"
#include "common/utils.h"
#include "ch.h"
#include "drivers/driver_api.h"
#include "drivers/i2c.h"
#include "bsp/utils/bsp_io.h"
#include <string.h>

static void lcd2004_send_command_or_data(device_t* dev, uint8_t value, uint8_t mode);
static void lcd2004_write4bits(device_t* dev, uint8_t value);
static void lcd2004_expand_and_write(device_t* dev, uint8_t value);
static void lcd2004_pulse_enable(device_t* dev, uint8_t value);

static const lcd2004_config_t lcd2004_config = {
    .addr = 0x27,
    .backlight = LCD_BACKLIGHT
};

static void lcd2004_internal_set_cursor(device_t* dev, uint8_t col, uint8_t row) {
    const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    lcd2004_send_command_or_data(dev, LCD_CMD_SET_DDRAM_ADDR | (col + row_offsets[row]), 0);
}

static int lcd2004_internal_write_string(device_t* dev, const char* str) {
    size_t count = 0;
    while (*str) {
        lcd2004_send_command_or_data(dev, *str++, 1);
        count++;
    }
    return count;
}


int lcd2004_init(device_t* dev) {
    bsp_printf("LCD2004: Initializing...\n");
    lcd2004_t* i = (lcd2004_t*)dev->priv;
    i->config = &lcd2004_config;

    uint8_t rx_buf;
    msg_t status = i2c_master_receive((I2CDriver *)dev->bus, i->config->addr, &rx_buf, 1);

    if (status != MSG_OK) {
        bsp_printf("LCD2004: No device found at address 0x%02X\n", i->config->addr);
        return -1;
    } else {
        bsp_printf("LCD2004: Found device at address 0x%02X\n", i->config->addr);
    }

    bsp_printf("LCD2004: Resetting display...\n");
    i->display_function = LCD_4BIT_MODE | LCD_2LINE | LCD_5x8_DOTS;
    chThdSleepMilliseconds(50);

    // Initial sequence to put LCD into 4-bit mode
    lcd2004_send_command_or_data(dev, 0x03, 0);
    chThdSleepMilliseconds(5);
    lcd2004_send_command_or_data(dev, 0x03, 0);
    chThdSleepMilliseconds(1);
    lcd2004_send_command_or_data(dev, 0x03, 0);
    chThdSleepMilliseconds(1);
    lcd2004_send_command_or_data(dev, 0x02, 0); // Set to 4-bit mode

    lcd2004_send_command_or_data(dev, LCD_CMD_FUNCTION_SET | i->display_function, 0);

    bsp_printf("LCD2004: Turning display OFF...\n");
    i->display_control = LCD_DISPLAY_OFF; // Turn display off
    lcd2004_send_command_or_data(dev, LCD_CMD_DISPLAY_CONTROL | i->display_control, 0);
    chThdSleepMilliseconds(1000); // Visible delay

    lcd2004_clear(dev); // Clear display
    i->display_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
    lcd2004_send_command_or_data(dev, LCD_CMD_ENTRY_MODE_SET | i->display_mode, 0);

    bsp_printf("LCD2004: Turning display ON...\n");
    i->display_control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF; // Turn display on
    lcd2004_send_command_or_data(dev, LCD_CMD_DISPLAY_CONTROL | i->display_control, 0);
    chThdSleepMilliseconds(1000); // Visible delay

    bsp_printf("LCD2004: Initialization complete.\n");
    return 0;
}

int lcd2004_write(device_t* dev, uint32_t offset, const void* buf, size_t count) {
    (void)offset; // Offset is ignored for now, always writes from current cursor
    return lcd2004_internal_write_string(dev, (const char*)buf);
}

int lcd2004_clear(device_t* dev) {
    lcd2004_send_command_or_data(dev, LCD_CMD_CLEAR_DISPLAY, 0);
    chThdSleepMilliseconds(2);
    return 0;
}

static void lcd2004_send_command_or_data(device_t* dev, uint8_t value, uint8_t mode) {
    uint8_t high_nibble = value & 0xF0;
    uint8_t low_nibble = (value << 4) & 0xF0;
    lcd2004_write4bits(dev, high_nibble | mode);
    lcd2004_write4bits(dev, low_nibble | mode);
}

static void lcd2004_write4bits(device_t* dev, uint8_t value) {
    lcd2004_expand_and_write(dev, value);
    lcd2004_pulse_enable(dev, value);
}

static void lcd2004_expand_and_write(device_t* dev, uint8_t value) {
    lcd2004_t* i = (lcd2004_t*)dev->priv;
    uint8_t buffer = value | i->config->backlight;
    i2c_master_transmit((I2CDriver *)dev->bus, i->config->addr, &buffer, 1, NULL, 0);
}

static void lcd2004_pulse_enable(device_t* dev, uint8_t value) {
    lcd2004_expand_and_write(dev, value | 0x04);
    chThdSleepMicroseconds(1);
    lcd2004_expand_and_write(dev, value & ~0x04);
    chThdSleepMicroseconds(50);
}

int lcd2004_ioctl(device_t* dev, uint32_t cmd, void* arg) {
    switch (cmd) {
        case LCD_IOCTL_SET_CURSOR:
        {
            lcd2004_cursor_pos_t* pos = (lcd2004_cursor_pos_t*)arg;
            lcd2004_internal_set_cursor(dev, pos->col, pos->row);
            break;
        }
        default:
            return -1;
    }
    return 0;
}

int32_t lcd2004_draw(device_t* dev, uint8_t page, uint8_t col, const uint8_t* buffer, size_t buffer_size) {
    if (page == 0xFF) { // Custom character creation
        uint8_t location = col;
        lcd2004_send_command_or_data(dev, LCD_CMD_SET_CGRAM_ADDR | (location << 3), 0);
        for (size_t i = 0; i < buffer_size; i++) {
            lcd2004_send_command_or_data(dev, buffer[i], 1);
        }
    } else { // Drawing text
        lcd2004_internal_set_cursor(dev, col, page);
        for (size_t i = 0; i < buffer_size; i++) {
            lcd2004_send_command_or_data(dev, buffer[i], 1);
        }
    }
    return 0;
}

const driver_t lcd2004_driver = {
    .name = "lcd2004",
    .init = lcd2004_init,
    .poll = NULL,
    .write = lcd2004_write,
    .clear = lcd2004_clear,
    .ioctl = lcd2004_ioctl,
    .draw = lcd2004_draw
};
