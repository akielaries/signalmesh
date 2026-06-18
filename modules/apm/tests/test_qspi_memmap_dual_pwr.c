#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/qspi_memmap.h"

#include "common/utils.h"

#define QSPI_MEMMAP_BASE   0x90000000UL
#define QSPI_FLASH_PATTERN 0xDEADBEEF

#define TEST_SECTOR_OFFSET 0x00000000UL
#define TEST_BYTES         256U

// d-cache is on and the mapped window is cacheable; keep buffers cache-line
// aligned so the invalidate over the mapped read does not touch neighbours
CC_ALIGN_DATA(32) static uint8_t s_pattern[TEST_BYTES];
CC_ALIGN_DATA(32) static uint8_t s_mapped_copy[TEST_BYTES];


// WSPI peripheral config: FSIZE = log2(8 MiB) - 1 = 22, CSHT = 1
static const WSPIConfig wspicfg = {
  .end_cb   = NULL,
  .error_cb = NULL,
  .dcr      = STM32_DCR_FSIZE(22U) | STM32_DCR_CSHT(1U),
};

static const qspi_memmap_config_t qspi_cfg = {
  .wspi       = &WSPID1,
  .wspi_cfg   = &wspicfg,
  .base       = QSPI_MEMMAP_BASE,
  .size_bytes = W25Q64_SIZE_BYTES,
  .lines      = W25QXX_QSPI_DUAL,   // 2-lane; set to W25QXX_QSPI_QUAD for 4-lane
};


int main(void) {
  bsp_init();
  bsp_printf("\n--- test_qspi_memmap_dual: memory-mapped QSPI proof ---\r\n");

  if (!qspi_memmap_init(&qspi_cfg)) {
    bsp_printf("FAIL: qspi_memmap_init (check wiring / JEDEC / QE)\n");
    return -1;
  }
  bsp_printf("init OK (dual, /16)\n");

  bsp_printf("reading back known pattern 0x%08lX\n", (unsigned long)QSPI_FLASH_PATTERN);

  // must enter memory-mapped mode before reading 0x90000000 via a pointer.
  // init() only brings the chip up (indirect mode); enable() maps the window
  const volatile uint8_t *flash = qspi_memmap_enable(&qspi_cfg);
  if (flash == NULL) {
    bsp_printf("FAIL: qspi_memmap_enable\n");
    return -1;
  }
  qspi_memmap_invalidate((const void *)flash, 32U);

  uint32_t w = *(const uint32_t *)flash;
  bsp_printf("mmap read @0x%08lX: 0x%08lX (%s)\n",
             (unsigned long)QSPI_MEMMAP_BASE, (unsigned long)w,
             (w == QSPI_FLASH_PATTERN) ? "persisted" : "MISMATCH");

  while (1) {
    chThdSleepMilliseconds(1000);
  }
}
