// memory-mapped QSPI proof using the qspi_memmap storage driver, dual (2-lane)
//
// this is the refactored, driver-based version of test_qspi_w25q64.c (kept as
// the low-level reference). all the QSPI/cache plumbing now lives in:
//   lib/drivers/.../w25qxx.{h,c}      command-level QSPI ops (w25qxx_qspi_*)
//   lib/drivers/.../qspi_memmap.{h,c} memory-mapped storage driver
//
// flip cfg.lines to W25QXX_QSPI_QUAD (and improve CLK signal integrity) for the
// 4-lane version; nothing else in this file needs to change
//
// build: add this file (and remove the other test main) in modules/apm/CMakeLists.txt
// requires HAL_USE_WSPI=TRUE (halconf.h), STM32_WSPI_USE_QUADSPI1=TRUE (mcuconf.h),
// and the QUADSPI pins configured (bsp/configs/bsp_spi_config.c)

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/qspi_memmap.h"

#include "common/utils.h"

#define QSPI_MEMMAP_BASE   0x90000000UL
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

// compare expected vs got, return number of differing bytes, report the first
static size_t report_cmp(const char *label, const uint8_t *exp,
                         const uint8_t *got, size_t len) {
  size_t first = len;
  size_t ndiff = 0;
  for (size_t i = 0; i < len; i++) {
    if (exp[i] != got[i]) {
      if (first == len) {
        first = i;
      }
      ndiff++;
    }
  }
  if (ndiff == 0) {
    bsp_printf("%s: OK all %u bytes match\n", label, (unsigned)len);
  } else {
    bsp_printf("%s: MISMATCH first at %u (0x%X) wrote %02X read %02X, %u/%u differ\n",
               label, (unsigned)first, (unsigned)first,
               exp[first], got[first], (unsigned)ndiff, (unsigned)len);
  }
  return ndiff;
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_qspi_memmap_dual: memory-mapped QSPI proof ---\r\n");

  if (!qspi_memmap_init(&qspi_cfg)) {
    bsp_printf("FAIL: qspi_memmap_init (check wiring / JEDEC / QE)\n");
    return -1;
  }
  bsp_printf("init OK (dual, /16)\n");

  // build and program a known pattern (indirect mode)
  for (size_t i = 0; i < TEST_BYTES; i++) {
    s_pattern[i] = (uint8_t)(0xA0 ^ (i & 0xFF));
  }
  if (!qspi_memmap_erase_sector(&qspi_cfg, TEST_SECTOR_OFFSET)) {
    bsp_printf("FAIL: erase\n");
    return -1;
  }
  if (!qspi_memmap_program(&qspi_cfg, TEST_SECTOR_OFFSET, s_pattern, TEST_BYTES)) {
    bsp_printf("FAIL: program\n");
    return -1;
  }
  bsp_printf("programmed %u bytes at 0x%08lX\n",
             (unsigned)TEST_BYTES, (unsigned long)TEST_SECTOR_OFFSET);

  // cross-check the write with a 1-line indirect read (known-good path)
  if (!w25qxx_qspi_read(qspi_cfg.wspi, W25QXX_QSPI_SINGLE,
                        TEST_SECTOR_OFFSET, s_mapped_copy, TEST_BYTES)) {
    bsp_printf("FAIL: 1-line indirect read-back\n");
    return -1;
  }
  report_cmp("indirect read-back", s_pattern, s_mapped_copy, TEST_BYTES);

  // memory-mapped read, repeated to expose any flakiness; invalidate each pass
  // so every read comes from the flash and not a stale d-cache line
  const volatile uint8_t *mapped = qspi_memmap_enable(&qspi_cfg);
  if (mapped == NULL) {
    bsp_printf("FAIL: qspi_memmap_enable\n");
    return -1;
  }

  unsigned passes = 0;
  for (unsigned pass = 0; pass < 10U; pass++) {
    qspi_memmap_invalidate((const void *)mapped, TEST_BYTES);
    for (size_t i = 0; i < TEST_BYTES; i++) {
      s_mapped_copy[i] = mapped[i];
    }
    if (memcmp(s_pattern, s_mapped_copy, TEST_BYTES) == 0) {
      passes++;
      bsp_printf("pass %u: OK\n", pass);
    } else {
      report_cmp("pass mismatch", s_pattern, s_mapped_copy, TEST_BYTES);
      print_hexdump("mapped", s_mapped_copy, 32);
    }
    chThdSleepMilliseconds(100);
  }

  qspi_memmap_disable(&qspi_cfg);

  if (passes != 10U) {
    bsp_printf("FAIL: %u/10 memory-mapped reads matched\n", passes);
    return -1;
  }
  bsp_printf("PASS: 10/10 memory-mapped reads matched at 0x%08lX\n",
             (unsigned long)(QSPI_MEMMAP_BASE + TEST_SECTOR_OFFSET));
  print_hexdump("mapped (head)", s_mapped_copy, 32);

  bsp_printf("\n--- test_qspi_memmap_dual finished ---\r\n");
  while (1) {
    chThdSleepMilliseconds(1000);
  }
}
