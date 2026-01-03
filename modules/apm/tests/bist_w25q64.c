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
#define FLASH_TOTAL_SIZE W25Q64_SIZE_BYTES
#define WRITE_CHUNK_SIZE W25QXX_PAGE_SIZE_BYTES // use page size for writing (256 bytes)
#define READ_CHUNK_SIZE  256                    // read in larger chunks


int main(void) {
  bsp_init();
  chThdSleepMilliseconds(1000); // wait for things to settle

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

  // local buffers for chunked R/W and verification
  uint8_t chunk_write_buf[WRITE_CHUNK_SIZE];
  uint8_t chunk_read_buf[WRITE_CHUNK_SIZE];

  // no longer generating a full 8MB buffer. Data is generated per chunk.
  bsp_printf("\nGenerating test data pattern for %lu bytes...\n", (unsigned long)FLASH_TOTAL_SIZE);
  // example data pattern for a chunk (first chunk to print hexdump)
  for (size_t i = 0; i < WRITE_CHUNK_SIZE; i++) {
    chunk_write_buf[i] = (uint8_t)(i % 256);
  }
  bsp_printf("Example write data for first chunk:\n");
  print_hexdump("Chunk Write data (head)", chunk_write_buf, 32);


  bsp_printf("\nWriting %lu bytes to Flash in %u-byte chunks...\n",
             (unsigned long)FLASH_TOTAL_SIZE,
             WRITE_CHUNK_SIZE);
  systime_t write_start_time = chVTGetSystemTimeX();
  int32_t total_written      = 0;

  for (uint32_t offset = 0; offset < FLASH_TOTAL_SIZE; offset += WRITE_CHUNK_SIZE) {
    // generate data pattern for the current chunk
    for (size_t i = 0; i < WRITE_CHUNK_SIZE; i++) {
      chunk_write_buf[i] = (uint8_t)((offset + i) % 256);
    }

    int32_t bytes_res = flash->driver->write(flash, offset, chunk_write_buf, WRITE_CHUNK_SIZE);
    if (bytes_res < 0) {
      bsp_printf("ERROR: Write failed at offset %lu, result %ld\n",
                 (unsigned long)offset,
                 bytes_res);
      return -1;
    }
    total_written += bytes_res;
    if ((offset % (FLASH_TOTAL_SIZE / 10)) == 0) { // print progress every 10%
      bsp_printf("  Written %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)FLASH_TOTAL_SIZE);
    }
  }
  systime_t write_end_time   = chVTGetSystemTimeX();
  uint32_t write_duration_ms = TIME_MS2I(write_end_time - write_start_time);
  bsp_printf("Total bytes written: %ld\n", total_written);
  bsp_printf("Write duration: %lu ms\n", (unsigned long)write_duration_ms);
  if (write_duration_ms > 0) {
    bsp_printf("Write speed: %.2f KB/s\n", (float)total_written / write_duration_ms);
  }

  return 0;
  bsp_printf("\nReading %lu bytes from Flash in %u-byte chunks and verifying...\n",
             (unsigned long)FLASH_TOTAL_SIZE,
             READ_CHUNK_SIZE);
  systime_t read_start_time = chVTGetSystemTimeX();
  int32_t total_read        = 0;
  bool verification_failed  = false;

  for (uint32_t offset = 0; offset < FLASH_TOTAL_SIZE; offset += READ_CHUNK_SIZE) {
    // generate expected data for the current chunk for verification
    for (size_t i = 0; i < READ_CHUNK_SIZE; i++) {
      chunk_write_buf[i] =
        (uint8_t)((offset + i) % 256); // re-use chunk_write_buf for expected data
    }

    int32_t bytes_res = flash->driver->read(flash, offset, chunk_read_buf, READ_CHUNK_SIZE);
    if (bytes_res < 0) {
      bsp_printf("ERROR: Read failed at offset %lu, result %ld\n",
                 (unsigned long)offset,
                 bytes_res);
      verification_failed = true;
      break;
    }
    total_read += bytes_res;

    // verify chunk immediately
    if (memcmp(chunk_write_buf, chunk_read_buf, READ_CHUNK_SIZE) != 0) {
      bsp_printf("VERIFICATION FAILURE: Mismatch at offset %lu!\n", (unsigned long)offset);
      // print discrepancy for the first few bytes of the mismatching chunk
      print_hexdump("Expected (first 32 bytes)", chunk_write_buf, 32);
      print_hexdump("Received (first 32 bytes)", chunk_read_buf, 32);
      verification_failed = true;
      break;
    }

    if ((offset % (FLASH_TOTAL_SIZE / 10)) == 0) { // print progress every 10%
      bsp_printf("  Read %lu / %lu bytes...\n",
                 (unsigned long)offset,
                 (unsigned long)FLASH_TOTAL_SIZE);
    }
  }
  systime_t read_end_time   = chVTGetSystemTimeX();
  uint32_t read_duration_ms = TIME_MS2I(read_end_time - read_start_time);
  bsp_printf("Total bytes read: %ld\n", total_read);
  bsp_printf("Read duration: %lu ms\n", (unsigned long)read_duration_ms);
  if (read_duration_ms > 0) {
    bsp_printf("Read speed: %.2f KB/s\n", (float)total_read / read_duration_ms);
  }

  if (!verification_failed) {
    bsp_printf("VERIFICATION SUCCESS: Data read back matches data written.\n");
  } else {
    bsp_printf("VERIFICATION FAILED: Mismatches found during read operation.\n");
  }

  bsp_printf("\n--- W25Q64 BIST Finished ---\r\n");

  return 0;
}
