#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/w25qxx.h" // for W25QXX defines and driver

#include "common/utils.h"


// define the total size of the Flash and the chunk size for read/write
#define FLASH_TOTAL_SIZE (W25Q64_SIZE_BYTES / 128)
#define WRITE_CHUNK_SIZE W25QXX_PAGE_SIZE_BYTES // use page size for writing (256 bytes)
#define READ_CHUNK_SIZE  256                    // read in larger chunks



// Static buffer for the entire test pattern
static uint8_t s_test_pattern_buffer[FLASH_TOTAL_SIZE];

int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting W25Q64 BIST ---\r\n");

  device_t *flash = find_device("w25qxx");
  if (flash == NULL || flash->driver == NULL) {
    bsp_printf("W25QXX device or driver not found!\n");
    return -1;
  }

  if (flash->driver->write == NULL || flash->driver->read == NULL) {
    bsp_printf("W25QXX driver does not support read/write operations.\n");
    return -1;
  }

  // local buffer for reading flash chunks into
  uint8_t chunk_read_buf[READ_CHUNK_SIZE];

  // Pre-generate the entire test pattern into the static buffer once.
  bsp_printf("\nGenerating test data pattern for %lu bytes...\n", (unsigned long)FLASH_TOTAL_SIZE);
  for (size_t i = 0; i < FLASH_TOTAL_SIZE; i++) {
    s_test_pattern_buffer[i] = (uint8_t)(i % 256);
  }
  bsp_printf("Example write data for first chunk:\n");
  print_hexdump("Chunk Write data (head)", s_test_pattern_buffer, 32);


  bsp_printf("\nWriting %lu bytes to Flash in %u-byte chunks...\n",
             (unsigned long)FLASH_TOTAL_SIZE,
             WRITE_CHUNK_SIZE);

  int32_t total_written      = 0;
  systime_t write_start_time = chVTGetSystemTimeX();

  for (uint32_t offset = 0; offset < FLASH_TOTAL_SIZE; offset += WRITE_CHUNK_SIZE) {
    // Data source is now the pre-filled static buffer
    int32_t bytes_res = flash->driver->write(flash, offset, s_test_pattern_buffer + offset, WRITE_CHUNK_SIZE);
    if (bytes_res < 0) {
      bsp_printf("ERROR: Write failed at offset %lu, result %ld\n",
                 (unsigned long)offset,
                 bytes_res);
      return -1;
    }
    total_written += bytes_res;
    /*
    if ((offset % (FLASH_TOTAL_SIZE / 10)) == 0) { // print progress every 10%
      bsp_printf("  Written %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)FLASH_TOTAL_SIZE);
    }
    */
  }
  
  systime_t write_end_time   = chVTGetSystemTimeX();
  uint32_t write_duration_ms = (write_end_time - write_start_time) / 10;

  bsp_printf("Total bytes written: %ld\n", total_written);
  bsp_printf("Write duration: %lu ms\n", (unsigned long)write_duration_ms);
  if (write_duration_ms > 0) {
    bsp_printf("Write speed: %.2f KB/s (%.2f MB/s)\n",
                ((float)total_written / 1024.0f) / ((float)write_duration_ms / 1000.0f),
                ((float)total_written / 1024.0f / 1024.0f) / ((float)write_duration_ms / 1000.0f));
  }

  bsp_printf("\nReading %lu bytes from Flash in %u-byte chunks and verifying...\n",
             (unsigned long)FLASH_TOTAL_SIZE,
             READ_CHUNK_SIZE);
  systime_t read_start_time = chVTGetSystemTimeX();
  int32_t total_read        = 0;
  bool verification_failed  = false;

  for (uint32_t offset = 0; offset < FLASH_TOTAL_SIZE; offset += READ_CHUNK_SIZE) {
    int32_t bytes_res = flash->driver->read(flash, offset, chunk_read_buf, READ_CHUNK_SIZE);
    if (bytes_res < 0) {
      bsp_printf("ERROR: Read failed at offset %lu, result %ld\n",
                 (unsigned long)offset,
                 bytes_res);
      verification_failed = true;
      break;
    }
    total_read += bytes_res;

    // verify chunk immediately against the pre-filled static buffer
    if (memcmp(s_test_pattern_buffer + offset, chunk_read_buf, READ_CHUNK_SIZE) != 0) {
      bsp_printf("VERIFICATION FAILURE: Mismatch at offset %lu!\n", (unsigned long)offset);
      print_hexdump("Expected (first 32 bytes)", s_test_pattern_buffer + offset, 32);
      print_hexdump("Received (first 32 bytes)", chunk_read_buf, 32);
      verification_failed = true;
      break;
    }

    /*
    if ((offset % (FLASH_TOTAL_SIZE / 10)) == 0) { // print progress every 10%
      bsp_printf("  Read %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)FLASH_TOTAL_SIZE);
    }
    */
  }
  systime_t read_end_time   = chVTGetSystemTimeX();
  uint32_t read_duration_ms = (read_end_time - read_start_time) / 10;
  bsp_printf("Total bytes read: %ld\n", total_read);
  bsp_printf("Read duration: %lu ms\n", (unsigned long)read_duration_ms);
  if (read_duration_ms > 0) {
    bsp_printf("Read speed: %.2f KB/s (%.2f MB/s)\n",
                ((float)total_read / 1024.0f) / ((float)read_duration_ms / 1000.0f),
                ((float)total_read / 1024.0f / 1024.0f) / ((float)read_duration_ms / 1000.0f));
  }

  if (!verification_failed) {
    bsp_printf("VERIFICATION SUCCESS: Data read back matches data written.\n");
  } else {
    bsp_printf("VERIFICATION FAILED: Mismatches found during read operation.\n");
  }

  bsp_printf("\n--- W25Q64 BIST Finished ---\r\n");

  return 0;
}
