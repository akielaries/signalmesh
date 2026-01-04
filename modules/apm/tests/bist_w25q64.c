#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/w25qxx.h" // for W25QXX defines and driver

#include "common/utils.h"

#define TEST_BUFFER_SIZE 32
#define FLASH_TOTAL_SIZE TEST_BUFFER_SIZE
#define WRITE_CHUNK_SIZE TEST_BUFFER_SIZE
#define READ_CHUNK_SIZE TEST_BUFFER_SIZE


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
  uint8_t chunk_write_buf[TEST_BUFFER_SIZE];
  uint8_t chunk_read_buf[TEST_BUFFER_SIZE];

  uint32_t offset = 0;
  bsp_printf("\nGenerating test data pattern for %lu bytes...\n", (unsigned long)TEST_BUFFER_SIZE);
  for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
    chunk_write_buf[i] = (uint8_t)((offset + i) % 256);
  }
  print_hexdump("Chunk Write data (head)", chunk_write_buf, TEST_BUFFER_SIZE);

  bsp_printf("\nWriting %lu bytes to Flash...\n",
             (unsigned long)TEST_BUFFER_SIZE);
  int32_t bytes_res = flash->driver->write(flash, offset, chunk_write_buf, TEST_BUFFER_SIZE);
  if (bytes_res < 0) {
    bsp_printf("ERROR: Write failed at offset %lu, result %ld\n",
               (unsigned long)offset,
               bytes_res);
    return -1;
  }

  offset = 0; // Reset offset for reading

  bsp_printf("\nReading %lu bytes from Flash and verifying...\n",
             (unsigned long)TEST_BUFFER_SIZE);
  // generate expected data for the current chunk for verification
  for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
    chunk_write_buf[i] = (uint8_t)((offset + i) % 256); // re-use chunk_write_buf for expected data
  }

  bytes_res = flash->driver->read(flash, offset, chunk_read_buf, TEST_BUFFER_SIZE);
  if (bytes_res < 0) {
    bsp_printf("ERROR: Read failed at offset %lu, result %ld\n",
               (unsigned long)offset,
               bytes_res);
    return -1;
  }

  // verify chunk immediately
  if (memcmp(chunk_write_buf, chunk_read_buf, TEST_BUFFER_SIZE) != 0) {
    bsp_printf("VERIFICATION FAILURE: Mismatch at offset %lu!\n", (unsigned long)offset);
    print_hexdump("Expected (first 32 bytes)", chunk_write_buf, TEST_BUFFER_SIZE);
    print_hexdump("Received (first 32 bytes)", chunk_read_buf, TEST_BUFFER_SIZE);
    bsp_printf("VERIFICATION FAILED: Mismatches found during read operation.\n");
    return -1;
  }


  bsp_printf("VERIFICATION SUCCESS: Data read back matches data written.\n");

  bsp_printf("\n--- W25Q64 BIST Finished ---\r\n");

  return 0;
}
