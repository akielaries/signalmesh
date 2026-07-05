#include "bootloader/crc32.h"

// lazily-built 256-entry table; 1KB in ram, avoids a giant literal and keeps
// throughput up when crc'ing megabytes. note: the stm32h7 has a hardware CRC
// unit we can swap in later if this shows up on a profile
static uint32_t table[256];
static int table_ready = 0;

static void build_table(void) {
  for (uint32_t i = 0; i < 256U; i++) {
    uint32_t c = i;
    for (int k = 0; k < 8; k++) {
      c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
    }
    table[i] = c;
  }
  table_ready = 1;
}

uint32_t bl_crc32_update(uint32_t crc, const void *data, size_t len) {
  if (!table_ready) {
    build_table();
  }
  const uint8_t *p = (const uint8_t *)data;
  for (size_t i = 0; i < len; i++) {
    crc = table[(crc ^ p[i]) & 0xFFU] ^ (crc >> 8);
  }
  return crc;
}

uint32_t bl_crc32(const void *data, size_t len) {
  return bl_crc32_final(bl_crc32_update(BL_CRC32_INIT, data, len));
}
