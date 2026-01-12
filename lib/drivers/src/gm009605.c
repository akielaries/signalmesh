#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/gm009605.h"
#include "ch.h"
#include "hal.h"
#include "drivers/font5x7.h"


// forward declarations for static functions
static int gm009605_send_cmd(gm009605_t *oled, uint8_t cmd);
static int gm009605_send_data(gm009605_t *oled, const uint8_t *data, size_t size);
static int gm009605_init(device_t *dev);
static int gm009605_clear(device_t *dev);
static int gm009605_write(device_t *dev, uint32_t offset, const void *buf, size_t count);
static int gm009605_ioctl(device_t *dev, uint32_t cmd, void *arg);
static int32_t gm009605_draw(device_t *dev, uint8_t page, uint8_t col, const uint8_t *buffer, size_t buffer_size);


const driver_t gm009605_driver __attribute__((used)) = {
  .name               = "gm009605",
  .init               = gm009605_init,
  .poll               = NULL,
  .ioctl              = (int (*)(device_id_t, uint32_t, void *))gm009605_ioctl,
  .readings_directory = NULL,
  .clear              = gm009605_clear,
  .write              = gm009605_write,
  .draw               = gm009605_draw,
};

static int gm009605_send_cmd(gm009605_t *oled, uint8_t cmd) {
  uint8_t buf[] = {0x00, cmd};
  msg_t ret     = i2c_master_transmit(oled->bus.i2c, oled->bus.addr, buf, sizeof(buf), NULL, 0);
  if (ret != MSG_OK) {
    i2c_handle_error(oled->bus.i2c, "gm009605_send_cmd");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

static int gm009605_send_data(gm009605_t *oled, const uint8_t *data, size_t size) {
  uint8_t buf[size + 1];
  buf[0] = 0x40;
  memcpy(buf + 1, data, size);
  msg_t ret = i2c_master_transmit(oled->bus.i2c, oled->bus.addr, buf, size + 1, NULL, 0);
  if (ret != MSG_OK) {
    i2c_handle_error(oled->bus.i2c, "gm009605_send_data");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

static int gm009605_clear(device_t *dev) {
  gm009605_t *oled = (gm009605_t *)dev->priv;
    uint8_t buffer[OLED_VISIBLE_WIDTH];
    memset(buffer, 0, sizeof(buffer));

    for (uint8_t page = 0; page < OLED_HEIGHT / 8; page++) {
        gm009605_send_cmd(oled, 0xB0 + page);
        gm009605_send_cmd(oled, 0x00 + (OLED_X_OFFSET & 0x0F));
        gm009605_send_cmd(oled, 0x10 + (OLED_X_OFFSET >> 4));
        gm009605_send_data(oled, buffer, sizeof(buffer));
    }

  return DRIVER_OK;
}

static int gm009605_write(device_t *dev, uint32_t offset, const void *buf, size_t count) {
  gm009605_t *oled = (gm009605_t *)dev->priv;
  const char *str  = (const char *)buf;

  uint8_t page = (offset >> 8) & 0xFF;
  uint8_t col  = offset & 0xFF;

  gm009605_send_cmd(oled, 0xB0 + page);
  gm009605_send_cmd(oled, 0x00 + (col & 0x0F));
  gm009605_send_cmd(oled, 0x10 + (col >> 4));

  for (size_t i = 0; i < count; i++) {
    if (str[i] >= FONT_FIRST_CHAR && str[i] <= FONT_LAST_CHAR) {
      const uint8_t *font_char = font5x7[str[i] - FONT_FIRST_CHAR];
      gm009605_send_data(oled, font_char, FONT_WIDTH);
      uint8_t space = 0;
      gm009605_send_data(oled, &space, 1);
    }
  }
  return count;
}

static int32_t gm009605_draw(device_t *dev, uint8_t page, uint8_t col, const uint8_t *buffer, size_t buffer_size) {
    gm009605_t *oled = (gm009605_t *)dev->priv;

    if (buffer_size > OLED_VISIBLE_WIDTH) {
        buffer_size = OLED_VISIBLE_WIDTH;
    }

    uint8_t hw_col = col + OLED_X_OFFSET;

    gm009605_send_cmd(oled, 0xB0 + page);
    gm009605_send_cmd(oled, 0x00 + (hw_col & 0x0F));
    gm009605_send_cmd(oled, 0x10 + (hw_col >> 4));

    gm009605_send_data(oled, buffer, buffer_size);

    return DRIVER_OK;
}


static int gm009605_ioctl(device_t *dev, uint32_t cmd, void *arg) {
    gm009605_t *oled = (gm009605_t *)dev->priv;
    uint8_t buffer[OLED_WIDTH];

    switch (cmd) {
        case OLED_IOCTL_TEST_PATTERN_HZTL_RAMP:
            for (uint8_t i = 0; i < OLED_HEIGHT / 8; i++) {
                for (uint8_t j = 0; j < OLED_WIDTH; j++) {
                    buffer[j] = j;
                }
                gm009605_send_cmd(oled, 0xB0 + i);
                gm009605_send_cmd(oled, 0x00);
                gm009605_send_cmd(oled, 0x10);
                gm009605_send_data(oled, buffer, sizeof(buffer));
            }
            return DRIVER_OK;
        case OLED_IOCTL_TEST_PATTERN_VERT_RAMP:
            for (uint8_t i = 0; i < OLED_HEIGHT / 8; i++) {
                for (uint8_t j = 0; j < OLED_WIDTH; j++) {
                    buffer[j] = (i * (OLED_WIDTH / 8)) + j;
                }
                gm009605_send_cmd(oled, 0xB0 + i);
                gm009605_send_cmd(oled, 0x00);
                gm009605_send_cmd(oled, 0x10);
                gm009605_send_data(oled, buffer, sizeof(buffer));
            }
            return DRIVER_OK;
        case OLED_IOCTL_SET_BRIGHTNESS:
            if (arg == NULL) {
                return DRIVER_INVALID_PARAM;
            }
            uint8_t brightness = *(uint8_t *)arg;
            gm009605_send_cmd(oled, SSD1306_SET_CONTRAST);
            gm009605_send_cmd(oled, brightness);
            return DRIVER_OK;
        default:
            return DRIVER_INVALID_PARAM;
    }
}
static int gm009605_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("GM009605 init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  gm009605_t *oled = (gm009605_t *)dev->priv;

  bsp_printf("GM009605: Initializing...\n");

  oled->bus.i2c     = (I2CDriver *)dev->bus;
  oled->bus.addr    = GM009605_DEFAULT_ADDRESS;
  oled->initialized = false;

  chThdSleepMilliseconds(100);

  bsp_printf("gm009605 Turning off\n");
  gm009605_send_cmd(oled, SSD1306_DISPLAY_OFF);
  chThdSleepMilliseconds(1000);

  gm009605_send_cmd(oled, SSD1306_SET_DISPLAY_CLOCK_DIV);
  gm009605_send_cmd(oled, 0x80);
  gm009605_send_cmd(oled, SSD1306_SET_MULTIPLEX);
  gm009605_send_cmd(oled, 0x3F);
  gm009605_send_cmd(oled, SSD1306_SET_DISPLAY_OFFSET);
  gm009605_send_cmd(oled, 0x00);
  gm009605_send_cmd(oled, SSD1306_SET_START_LINE | 0x0);
  gm009605_send_cmd(oled, SSD1306_CHARGE_PUMP);
  gm009605_send_cmd(oled, 0x14);
  gm009605_send_cmd(oled, SSD1306_MEMORY_MODE);
  gm009605_send_cmd(oled, 0x00);
  gm009605_send_cmd(oled, SSD1306_SEGRE_MAP | 0x1);
  gm009605_send_cmd(oled, SSD1306_COM_SCAN_DEC);
  gm009605_send_cmd(oled, SSD1306_SET_COM_PINS);
  gm009605_send_cmd(oled, 0x12);
  gm009605_send_cmd(oled, SSD1306_SET_CONTRAST);
  gm009605_send_cmd(oled, 0xCF);
  gm009605_send_cmd(oled, SSD1306_SET_PRECHARGE);
  gm009605_send_cmd(oled, 0xF1);
  gm009605_send_cmd(oled, SSD1306_SET_VCOM_DETECT);
  gm009605_send_cmd(oled, 0x40);
  gm009605_send_cmd(oled, SSD1306_DISPLAY_ALL_ON_RESUME);
  gm009605_send_cmd(oled, SSD1306_NORMAL_DISPLAY);

  bsp_printf("gm009605 Turning on\n");
  gm009605_send_cmd(oled, SSD1306_DISPLAY_ON);

  oled->initialized = true;

  return DRIVER_OK;
}



