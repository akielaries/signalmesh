#include <string.h>
#include <stdbool.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/driver_api.h"
#include "drivers/driver_registry.h"
#include "drivers/w25qxx.h" // for W25QXX defines and driver

#include "common/utils.h"

// Define a smaller test size to keep the test duration reasonable
#define FLASH_TEST_SIZE (W25Q64_SIZE_BYTES / 128)
#define CHUNK_SIZE 256 // Use a common chunk size for reads and writes

// Static buffers for the two unique test patterns
static uint8_t s_test_pattern_1[FLASH_TEST_SIZE];
static uint8_t s_test_pattern_2[FLASH_TEST_SIZE];


/**
 * @brief Performs a full write/read/verify and timing test on a single flash device.
 * @param flash Pointer to the flash device object.
 * @param device_name Name of the device for logging.
 * @param test_pattern The data pattern to use for the test.
 * @return true if the test passed, false otherwise.
 */
bool test_single_flash(device_t *flash, const char *device_name, const uint8_t *test_pattern) {
  bsp_printf("\n--- Testing Flash Device: %s ---\n", device_name);
  print_hexdump("Write data (first 32 bytes)", test_pattern, 32);

  // write test
  bsp_printf("Writing %lu bytes...\n", (unsigned long)FLASH_TEST_SIZE);
  int32_t total_written = 0;
  systime_t write_start_time = chVTGetSystemTimeX();

  for (uint32_t offset = 0; offset < FLASH_TEST_SIZE; offset += CHUNK_SIZE) {
    if (flash->driver->write(flash, offset, test_pattern + offset, CHUNK_SIZE) < 0) {
      bsp_printf("ERROR: Write failed on %s at offset %lu\n", device_name, (unsigned long)offset);
      return false;
    }
    total_written += CHUNK_SIZE;
  }

  systime_t write_end_time = chVTGetSystemTimeX();
  uint32_t write_duration_ms = (write_end_time - write_start_time) / 10;

  bsp_printf("Write duration: %lu ms\n", (unsigned long)write_duration_ms);
  if (write_duration_ms > 0) {
    bsp_printf("Speed: %.2f KB/s\n",
               ((float)total_written / 1024.0f) / ((float)write_duration_ms / 1000.0f));
  } else {
    bsp_printf("\n");
  }

  // read and verif
  bsp_printf("Reading and verifying %lu bytes...\n", (unsigned long)FLASH_TEST_SIZE);
  uint8_t read_buf[CHUNK_SIZE];
  bool verification_failed = false;
  int32_t total_read = 0;
  systime_t read_start_time = chVTGetSystemTimeX();

  for (uint32_t offset = 0; offset < FLASH_TEST_SIZE; offset += CHUNK_SIZE) {
    if (flash->driver->read(flash, offset, read_buf, CHUNK_SIZE) < 0) {
      bsp_printf("ERROR: Read failed on %s at offset %lu\n", device_name, (unsigned long)offset);
      return false;
    }
    if (memcmp(test_pattern + offset, read_buf, CHUNK_SIZE) != 0) {
      bsp_printf("VERIFICATION FAILURE: Mismatch on %s at offset %lu!\n", device_name, (unsigned long)offset);
      print_hexdump("Expected", test_pattern + offset, 32);
      print_hexdump("Received", read_buf, 32);
      return false;
    }

    if (offset == 0) { // Only print the first chunk of received data
      bsp_printf("Received data (first 32 bytes):\n");
      print_hexdump("Received", read_buf, 32);
    }

    total_read += CHUNK_SIZE;
  }

  systime_t read_end_time = chVTGetSystemTimeX();
  uint32_t read_duration_ms = (read_end_time - read_start_time) / 10;
  
  bsp_printf("Read duration: %lu ms\n", (unsigned long)read_duration_ms);
  if (read_duration_ms > 0) {
    bsp_printf("Speed: %.2f KB/s\n",
               ((float)total_read / 1024.0f) / ((float)read_duration_ms / 1000.0f));
  } else {
    bsp_printf("\n");
  }

  bsp_printf("VERIFICATION SUCCESS for %s.\n", device_name);
  return true;
}


int main(void) {
  bsp_init();

  bsp_printf("\n--- Starting Dual W25Q64 BIST ---\r\n");

  // get device bindings for both flash chips
  device_t *flash1 = find_device("w25qxx_1");
  device_t *flash2 = find_device("w25qxx_2");

  if (flash1 == NULL || flash2 == NULL) {
    bsp_printf("ERROR: Could not find one or both W25QXX devices!\n");
    return -1;
  }

  // generate two unique test patterns
  bsp_printf("Generating test data patterns...\n");
  for (size_t i = 0; i < FLASH_TEST_SIZE; i++) {
    s_test_pattern_1[i] = (uint8_t)(i % 256);        // pattern 1: 0, 1, 2, ...
    s_test_pattern_2[i] = (uint8_t)(255 - (i % 256)); // pattern 2: 255, 254, 253, ...
  }

  // test each flash chip sequentially
  bool success1 = test_single_flash(flash1, "w25qxx_1", s_test_pattern_1);
  bool success2 = test_single_flash(flash2, "w25qxx_2", s_test_pattern_2);

  bsp_printf("\n--- Dual W25Q64 BIST Finished ---\r\n");
  if (success1 && success2) {
    bsp_printf("Result: ALL TESTS PASSED\n");
  } else {
    bsp_printf("Result: ONE OR MORE TESTS FAILED\n");
  }

  return 0;
}
