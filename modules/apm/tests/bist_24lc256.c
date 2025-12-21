#include <string.h>
#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/24lc256.h" // For EEPROM_24LC256_SIZE_BYTES

// Define the total size of the EEPROM and the chunk size for read/write
#define EEPROM_TOTAL_SIZE EEPROM_24LC256_SIZE_BYTES
#define WRITE_CHUNK_SIZE  EEPROM_24LC256_PAGE_SIZE_BYTES // Use page size for writing
#define READ_CHUNK_SIZE   256                            // Read in larger chunks

// Buffers for write and read operations
static uint8_t write_buffer[EEPROM_TOTAL_SIZE];
static uint8_t read_buffer[EEPROM_TOTAL_SIZE];

// Helper to convert a byte to its two-character hex representation
static void byte_to_hex(uint8_t byte, char *hex_str) {
  const char hex_chars[] = "0123456789abcdef";
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

  bsp_printf("\n--- Starting EEPROM BIST (24LC256) ---\r\n");

  device_t *eeprom = find_device("24lc256");
  if (eeprom == NULL || eeprom->driver == NULL) {
    bsp_printf("EEPROM device or driver not found!\n");
    return -1;
  }

  if (eeprom->driver->write == NULL || eeprom->driver->read == NULL) {
    bsp_printf("EEPROM driver does not support read/write operations.\n");
    return -1;
  }

  // 1. Prepare data for writing
  bsp_printf("Generating %lu bytes of test data...\n", (unsigned long)EEPROM_TOTAL_SIZE);
  for (size_t i = 0; i < EEPROM_TOTAL_SIZE; i++) {
    write_buffer[i] = (uint8_t)(i % 256);
  }
  bsp_printf("First 32 bytes of write data:\n");
  print_hexdump("Write data (head)", write_buffer, 32);
  bsp_printf("Last 32 bytes of write data:\n");
  print_hexdump("Write data (tail)", &write_buffer[EEPROM_TOTAL_SIZE - 32], 32);

  // 2. Write data to EEPROM and time it
  bsp_printf("\nWriting %lu bytes to EEPROM in %u-byte chunks...\n",
             (unsigned long)EEPROM_TOTAL_SIZE,
             WRITE_CHUNK_SIZE);
  systime_t start_time  = chVTGetSystemTimeX();
  int32_t total_written = 0;

  for (uint32_t offset = 0; offset < EEPROM_TOTAL_SIZE; offset += WRITE_CHUNK_SIZE) {
    size_t chunk_to_write = (EEPROM_TOTAL_SIZE - offset < WRITE_CHUNK_SIZE)
                              ? (EEPROM_TOTAL_SIZE - offset)
                              : WRITE_CHUNK_SIZE;
    int32_t res = eeprom->driver->write(eeprom, offset, &write_buffer[offset], chunk_to_write);
    if (res < 0) {
      bsp_printf("ERROR: Write failed at offset %lu, result %ld\n", (unsigned long)offset, res);
      return -1;
    }
    total_written += res;
    // Optional: Print progress
    if ((offset % (EEPROM_TOTAL_SIZE / 10)) == 0) {
      bsp_printf("  Written %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)EEPROM_TOTAL_SIZE);
    }
  }
  systime_t end_time         = chVTGetSystemTimeX();
  uint32_t write_duration_ms = TIME_MS2I(end_time - start_time);
  bsp_printf("Total bytes written: %ld\n", total_written);
  bsp_printf("Write duration: %lu ms\n", (unsigned long)write_duration_ms);
  if (write_duration_ms > 0) {
    bsp_printf("Write speed: %.2f KB/s\n", (float)total_written / write_duration_ms);
  }

  // 3. Read data from EEPROM and time it
  bsp_printf("\nReading %lu bytes from EEPROM in %u-byte chunks...\n",
             (unsigned long)EEPROM_TOTAL_SIZE,
             READ_CHUNK_SIZE);
  start_time         = chVTGetSystemTimeX();
  int32_t total_read = 0;

  for (uint32_t offset = 0; offset < EEPROM_TOTAL_SIZE; offset += READ_CHUNK_SIZE) {
    size_t chunk_to_read = (EEPROM_TOTAL_SIZE - offset < READ_CHUNK_SIZE)
                             ? (EEPROM_TOTAL_SIZE - offset)
                             : READ_CHUNK_SIZE;
    int32_t res = eeprom->driver->read(eeprom, offset, &read_buffer[offset], chunk_to_read);
    if (res < 0) {
      bsp_printf("ERROR: Read failed at offset %lu, result %ld\n", (unsigned long)offset, res);
      return -1;
    }
    total_read += res;
    // Optional: Print progress
    if ((offset % (EEPROM_TOTAL_SIZE / 10)) == 0) {
      bsp_printf("  Read %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)EEPROM_TOTAL_SIZE);
    }
  }
  end_time                  = chVTGetSystemTimeX();
  uint32_t read_duration_ms = TIME_MS2I(end_time - start_time);
  bsp_printf("Total bytes read: %ld\n", total_read);
  bsp_printf("Read duration: %lu ms\n", (unsigned long)read_duration_ms);
  if (read_duration_ms > 0) {
    bsp_printf("Read speed: %.2f KB/s\n", (float)total_read / read_duration_ms);
  }

  // 4. Verify data integrity
  bsp_printf("\nVerifying data integrity...\n");
  if (memcmp(write_buffer, read_buffer, EEPROM_TOTAL_SIZE) == 0) {
    bsp_printf("VERIFICATION SUCCESS: Data read back matches data written.\n");
  } else {
    bsp_printf("VERIFICATION FAILURE: Data mismatch!\n");
    // Print first discrepancy
    for (size_t i = 0; i < EEPROM_TOTAL_SIZE; i++) {
      if (write_buffer[i] != read_buffer[i]) {
        bsp_printf("Mismatch at offset %lu: wrote 0x%02X, read 0x%02X\n",
                   (unsigned long)i,
                   write_buffer[i],
                   read_buffer[i]);
        break;
      }
    }
  }

  bsp_printf("\n--- EEPROM BIST Finished ---\r\n");

  return 0;
}
