#include <string.h>
#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"

#define TEST_BUFFER_SIZE   100
#define TEST_EEPROM_OFFSET 0

#define TEST_EEPROM_OFFSET 0

// Helper to convert a byte to its two-character hex representation
static void byte_to_hex(uint8_t byte, char *hex_str) {
  const char hex_chars[] = "0123456789ABCDEF";
  hex_str[0]             = hex_chars[(byte >> 4) & 0x0F];
  hex_str[1]             = hex_chars[byte & 0x0F];
}

void print_hexdump(const char *prefix, const uint8_t *data, size_t size) {
  // Max line length: "  0000: FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF |................|\n"
  //                   7  + (16 * 3) + 3 + 16 + 1 + 1 = 48 + 16 + 7 = 71, round up to 80 for safety
  char line_buf[80];
  char *ptr;

  bsp_printf("%s (%d bytes):\n", prefix, size);
  for (size_t i = 0; i < size; i += 16) {
    ptr = line_buf;
    // Print address
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "  %04X: ", i);

    // Print hex bytes
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        byte_to_hex(data[i + j], ptr);
        ptr += 2;
        *ptr++ = ' ';
      } else {
        *ptr++ = ' ';
        *ptr++ = ' ';
        *ptr++ = ' ';
      }
    }
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), " |");

    // Print ASCII representation
    for (size_t j = 0; j < 16; j++) {
      if (i + j < size) {
        uint8_t c = data[i + j];
        *ptr++    = (c >= 32 && c <= 126) ? c : '.';
      } else {
        *ptr++ = ' ';
      }
    }
    ptr += chsnprintf(ptr, sizeof(line_buf) - (ptr - line_buf), "|\n");
    bsp_printf("%s", line_buf);
  }
}

int main(void) {
  bsp_init();
  chThdSleepMilliseconds(1000); // Wait for things to settle

  bsp_printf("--- Starting Storage Test ---\r\n");

  device_t *eeprom = find_device("24lc256");
  if (eeprom == NULL || eeprom->driver == NULL) {
    bsp_printf("EEPROM device or driver not found!\n");
    return -1;
  }

  if (eeprom->driver->write == NULL || eeprom->driver->read == NULL) {
    bsp_printf("EEPROM driver does not support read/write operations.\n");
    return -1;
  }

  uint8_t write_buf[TEST_BUFFER_SIZE];
  uint8_t read_buf[TEST_BUFFER_SIZE];

  for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
    write_buf[i] = (uint8_t)(i % 256);
  }
  memset(read_buf, 0, TEST_BUFFER_SIZE);

  bsp_printf("Writing %d bytes to offset %d...\n", TEST_BUFFER_SIZE, TEST_EEPROM_OFFSET);
  print_hexdump("Write buffer", write_buf, TEST_BUFFER_SIZE);
  int32_t bytes_written =
    eeprom->driver->write(eeprom, TEST_EEPROM_OFFSET, write_buf, TEST_BUFFER_SIZE);
  if (bytes_written < 0) {
    bsp_printf("Failed to write to EEPROM.\n");
    return -1;
  }
  bsp_printf("Successfully wrote %d bytes.\n", bytes_written);

  bsp_printf("Reading %d bytes from offset %d...\n", TEST_BUFFER_SIZE, TEST_EEPROM_OFFSET);
  int32_t bytes_read = eeprom->driver->read(eeprom, TEST_EEPROM_OFFSET, read_buf, TEST_BUFFER_SIZE);
  if (bytes_read < 0) {
    bsp_printf("Failed to read from EEPROM.\n");
    return -1;
  }
  bsp_printf("Successfully read %d bytes.\n", bytes_read);
  print_hexdump("Read buffer", read_buf, TEST_BUFFER_SIZE);

  if (memcmp(write_buf, read_buf, TEST_BUFFER_SIZE) == 0) {
    bsp_printf("SUCCESS: Data read back matches data written.\n");
  } else {
    bsp_printf("FAILURE: Data mismatch!\n");
    for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
      if (write_buf[i] != read_buf[i]) {
        bsp_printf("Mismatch at index %d: wrote 0x%02X, read 0x%02X\n",
                   i,
                   write_buf[i],
                   read_buf[i]);
      }
    }
  }

  bsp_printf("\n--- Storage Test Finished ---\r\n");

  return 0;
}
