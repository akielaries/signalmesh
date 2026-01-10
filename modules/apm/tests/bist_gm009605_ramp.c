#include <string.h>
#include <math.h> // For sin function

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/gm009605.h"


void cycle_brightness(device_t *oled) {
  bsp_printf("Cycling Brightness\n");
  for (uint8_t i = 0; i < 255; i++) {
    bsp_printf("Increasing rightness to %d\n", i);
    oled->driver->ioctl(oled, OLED_IOCTL_SET_BRIGHTNESS, &i);
    chThdSleepMilliseconds(5);
  }
  for (uint8_t i = 255; i > 0; i--) {
    bsp_printf("Decreasing brightness to %d\n", i);
    oled->driver->ioctl(oled, OLED_IOCTL_SET_BRIGHTNESS, &i);
    chThdSleepMilliseconds(1);
  }
  uint8_t max_bright = 255;
  oled->driver->ioctl(oled, OLED_IOCTL_SET_BRIGHTNESS, &max_bright);

}

void draw_sine_wave(device_t *oled) {
  oled->driver->clear(oled);
  bsp_printf("Drawing Sine Wave\n");

  uint8_t buffer[OLED_WIDTH];
  float frequency = 0.1f; // Adjust for more or fewer cycles
  float amplitude = (OLED_HEIGHT / 2) - 1; // Max amplitude to fit display

  for (uint8_t page = 0; page < OLED_HEIGHT / 8; page++) {
    for (uint8_t col = 0; col < OLED_WIDTH; col++) {
      int y = (int)((OLED_HEIGHT / 2) + amplitude * sinf(col * frequency));
      
      // Calculate which bit in the current byte (column) corresponds to 'y'
      uint8_t byte_val = 0;
      if (y >= (page * 8) && y < ((page + 1) * 8)) {
        byte_val = 1 << (y % 8);
      }
      buffer[col] = byte_val;
    }
    oled->driver->draw(oled, page, 0, buffer, OLED_WIDTH);
  }
}

int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting GM009605/SSD1306 test ---\r\n");

  device_t *oled = find_device("gm009605");
  if (oled == NULL || oled->driver == NULL) {
    return -1;
  }

  if (oled->driver->ioctl == NULL) {
    return -1;
  }

  bsp_printf("Drawing Horizontal Ramp\n");
  oled->driver->ioctl(oled, OLED_IOCTL_TEST_PATTERN_HZTL_RAMP, NULL);
  chThdSleepMilliseconds(2000);

  cycle_brightness(oled);

  bsp_printf("Drawing Vertical Ramp\n");
  oled->driver->ioctl(oled, OLED_IOCTL_TEST_PATTERN_VERT_RAMP, NULL);
  chThdSleepMilliseconds(2000);

  draw_sine_wave(oled);
  chThdSleepMilliseconds(2000);

  return 0;
}
