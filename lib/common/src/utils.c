#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/utils/bsp_io.h"

#include "common/utils.h"


void print_hexdump(const char *prefix, const uint8_t *data, size_t size) {
  char line_buf[80];
  char *ptr;

  bsp_printf("%s (%u bytes):\r\n", prefix, size);
  for (size_t i = 0; i < size; i += 16) {
    ptr = line_buf;
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "  %04X: ", i);

    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        chsnprintf(ptr, 4, "%02X ", data[i + j]);
        ptr += 3;
      } else {
        chsnprintf(ptr, 4, "   ");
        ptr += 3;
      }
    }
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), " |");

    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        uint8_t c = data[i + j];
        *ptr++    = (c >= 32 && c <= 126) ? c : '.';
      } else {
        *ptr++ = ' ';
      }
    }
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "|\r\n");
    bsp_printf("%s", line_buf);
  }
}
