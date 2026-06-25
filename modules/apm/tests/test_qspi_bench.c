// QSPI flash characterization: capacity, mappable size, and r/w bandwidth
//
// uses the qspi_memmap storage driver + w25qxx QSPI ops. reports:
//   1. total flash size (decoded from JEDEC id, vs the configured size)
//   2. how much of it is memory-mappable (reads first word of each 1MB block)
//   3. read bandwidth, both memory-mapped and indirect
//   4. write bandwidth (erase + program), confined to a test region
//
// destructive ONLY in the write-test region (BENCH_WRITE_OFFSET .. +SIZE),
// which is 1MB into the device so the persistence magic at offset 0 is left
// intact. all other operations are read-only.
//
// timing uses the chibios tick (CH_CFG_ST_FREQUENCY, 100us resolution); the
// transfers are sized large enough (>= tens of ms) that this is plenty.

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/qspi_memmap.h"
#include "drivers/w25qxx.h"

#include "common/utils.h"

#define QSPI_MEMMAP_BASE     0x90000000UL

#define BENCH_MMAP_READ_BYTES (1024U * 1024U) // 1 MB memory-mapped read
#define BENCH_INDIR_READ_BYTES (256U * 1024U) // 256 KB indirect read
#define BENCH_WRITE_OFFSET    (1024U * 1024U) // 1 MB in: clear of the magic at 0
#define BENCH_WRITE_BYTES     (64U * 1024U)   // 64 KB erase + program

#define SECTOR_SIZE 4096U
#define PAGE_SIZE   256U

// working buffers (cache-line aligned, AXI SRAM)
CC_ALIGN_DATA(32) static uint8_t rbuf[4096];
CC_ALIGN_DATA(32) static uint8_t pat[PAGE_SIZE];

// keeps read loops from being optimized away
static volatile uint32_t g_sink;

static const WSPIConfig wspicfg = {
  .end_cb   = NULL,
  .error_cb = NULL,
  // FSIZE = log2(device_size) - 1; 16 MB W25Q128 -> 23
  .dcr      = STM32_DCR_FSIZE(23U) | STM32_DCR_CSHT(1U),
};

static const qspi_memmap_config_t qspi_cfg = {
  .wspi       = &WSPID1,
  .wspi_cfg   = &wspicfg,
  .base       = QSPI_MEMMAP_BASE,
  .size_bytes = W25Q128_SIZE_BYTES,
  .lines      = W25QXX_QSPI_DUAL,
};

static uint32_t elapsed_us(systime_t t0, systime_t t1) {
  uint32_t us = (uint32_t)TIME_I2US(chTimeDiffX(t0, t1));
  return us == 0U ? 1U : us;
}

static uint32_t kbps(size_t bytes, uint32_t us) {
  return (uint32_t)(((uint64_t)bytes * 1000000ULL) / (uint64_t)us / 1024ULL);
}

static const char *lines_name(w25qxx_qspi_lines_t lines) {
  switch (lines) {
    case W25QXX_QSPI_SINGLE:
      return "single";
    case W25QXX_QSPI_DUAL:
      return "dual";
    case W25QXX_QSPI_QUAD:
      return "quad";
    default:
      return "?";
  }
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_qspi_bench: QSPI capacity + bandwidth ---\r\n");

  if (!qspi_memmap_init(&qspi_cfg)) {
    bsp_printf("FAIL: qspi_memmap_init\n");
    return -1;
  }
  bsp_printf("init OK (%s, /%u)\n", lines_name(qspi_cfg.lines),
             (unsigned)STM32_WSPI_QUADSPI1_PRESCALER_VALUE);

  // ---- 1. capacity (decode from JEDEC id) -------------------------------
  uint8_t jedec[3];
  if (!w25qxx_qspi_read_jedec(qspi_cfg.wspi, jedec)) {
    bsp_printf("FAIL: read jedec\n");
    return -1;
  }
  // for winbond W25Q, capacity byte = log2(size in bytes)
  uint32_t detected = (jedec[2] >= 16U && jedec[2] <= 26U) ? (1UL << jedec[2]) : 0U;
  bsp_printf("JEDEC %02X %02X %02X -> detected size %lu bytes (%lu MB), configured %lu MB\n",
             jedec[0], jedec[1], jedec[2],
             (unsigned long)detected, (unsigned long)(detected / (1024UL * 1024UL)),
             (unsigned long)(qspi_cfg.size_bytes / (1024UL * 1024UL)));

  // ---- 2. write bandwidth (DESTRUCTIVE in the test region) --------------
  // build a deterministic page pattern
  for (uint32_t i = 0; i < PAGE_SIZE; i++) {
    pat[i] = (uint8_t)(i ^ 0x5AU);
  }

  // erase
  systime_t t0 = chVTGetSystemTimeX();
  for (uint32_t off = 0; off < BENCH_WRITE_BYTES; off += SECTOR_SIZE) {
    if (!qspi_memmap_erase_sector(&qspi_cfg, BENCH_WRITE_OFFSET + off)) {
      bsp_printf("FAIL: erase @ +0x%lX\n", (unsigned long)(BENCH_WRITE_OFFSET + off));
      return -1;
    }
  }
  uint32_t erase_us = elapsed_us(t0, chVTGetSystemTimeX());

  // program
  t0 = chVTGetSystemTimeX();
  for (uint32_t off = 0; off < BENCH_WRITE_BYTES; off += PAGE_SIZE) {
    if (!qspi_memmap_program(&qspi_cfg, BENCH_WRITE_OFFSET + off, pat, PAGE_SIZE)) {
      bsp_printf("FAIL: program @ +0x%lX\n", (unsigned long)(BENCH_WRITE_OFFSET + off));
      return -1;
    }
  }
  uint32_t prog_us = elapsed_us(t0, chVTGetSystemTimeX());

  // verify: read each page back through BOTH the single-line (0x03) and the
  // dual (0x3B) indirect paths and compare to the pattern. reading both lets
  // us tell a bad write (both mismatch at the same place) apart from a flaky
  // read path (only one mismatches). reports the first divergence per mode
  bool sgl_ok = true, dual_ok = true;
  for (uint32_t off = 0; off < BENCH_WRITE_BYTES && (sgl_ok || dual_ok); off += PAGE_SIZE) {
    uint8_t *sbuf = rbuf;       // bytes [0..255]   single read
    uint8_t *dbuf = rbuf + 256; // bytes [256..511] dual read
    if (sgl_ok && w25qxx_qspi_read(qspi_cfg.wspi, W25QXX_QSPI_SINGLE,
                                   BENCH_WRITE_OFFSET + off, sbuf, PAGE_SIZE)) {
      for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (sbuf[i] != pat[i]) {
          bsp_printf("verify single: MISMATCH @ +0x%06lX want 0x%02X got 0x%02X\n",
                     (unsigned long)(BENCH_WRITE_OFFSET + off + i), pat[i], sbuf[i]);
          sgl_ok = false;
          break;
        }
      }
    } else if (sgl_ok) {
      bsp_printf("verify single: read FAIL @ +0x%06lX\n",
                 (unsigned long)(BENCH_WRITE_OFFSET + off));
      sgl_ok = false;
    }
    if (dual_ok && w25qxx_qspi_read(qspi_cfg.wspi, qspi_cfg.lines,
                                    BENCH_WRITE_OFFSET + off, dbuf, PAGE_SIZE)) {
      for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (dbuf[i] != pat[i]) {
          bsp_printf("verify %-6s: MISMATCH @ +0x%06lX want 0x%02X got 0x%02X\n",
                     lines_name(qspi_cfg.lines),
                     (unsigned long)(BENCH_WRITE_OFFSET + off + i), pat[i], dbuf[i]);
          dual_ok = false;
          break;
        }
      }
    } else if (dual_ok) {
      bsp_printf("verify %-6s: read FAIL @ +0x%06lX\n",
                 lines_name(qspi_cfg.lines),
                 (unsigned long)(BENCH_WRITE_OFFSET + off));
      dual_ok = false;
    }
  }
  bool wr_ok = sgl_ok && dual_ok;
  bsp_printf("erase   : %lu KB in %lu us = %lu KB/s\n",
             (unsigned long)(BENCH_WRITE_BYTES / 1024U), (unsigned long)erase_us,
             (unsigned long)kbps(BENCH_WRITE_BYTES, erase_us));
  bsp_printf("program : %lu KB in %lu us = %lu KB/s\n",
             (unsigned long)(BENCH_WRITE_BYTES / 1024U), (unsigned long)prog_us,
             (unsigned long)kbps(BENCH_WRITE_BYTES, prog_us));
  bsp_printf("write verify: %s\n", wr_ok ? "OK" : "MISMATCH");

  // ---- 3. indirect read bandwidth (dual) --------------------------------
  uint32_t sum = 0;
  t0 = chVTGetSystemTimeX();
  for (uint32_t done = 0; done < BENCH_INDIR_READ_BYTES; done += sizeof rbuf) {
    if (!w25qxx_qspi_read(qspi_cfg.wspi, qspi_cfg.lines, done, rbuf, sizeof rbuf)) {
      bsp_printf("FAIL: indirect read @ +0x%lX\n", (unsigned long)done);
      return -1;
    }
    for (uint32_t j = 0; j < sizeof rbuf; j++) {
      sum += rbuf[j];
    }
  }
  uint32_t ind_us = elapsed_us(t0, chVTGetSystemTimeX());
  g_sink = sum;
  bsp_printf("read indirect (dual): %lu KB in %lu us = %lu KB/s\n",
             (unsigned long)(BENCH_INDIR_READ_BYTES / 1024U), (unsigned long)ind_us,
             (unsigned long)kbps(BENCH_INDIR_READ_BYTES, ind_us));

  // ---- 4. memory-mapped coverage + read bandwidth -----------------------
  const volatile uint8_t *flash = qspi_memmap_enable(&qspi_cfg);
  if (flash == NULL) {
    bsp_printf("FAIL: qspi_memmap_enable\n");
    return -1;
  }

  // coverage: first word of each 1MB block proves the whole window maps
  bsp_printf("memmap coverage (first word per 1MB):\n");
  for (uint32_t blk = 0; blk < qspi_cfg.size_bytes; blk += (1024U * 1024U)) {
    qspi_memmap_invalidate((const void *)(flash + blk), 32U);
    uint32_t w = *(const volatile uint32_t *)(flash + blk);
    bsp_printf("  +0x%06lX: 0x%08lX\n", (unsigned long)blk, (unsigned long)w);
  }

  // read-determinism probe: read ONE page repeatedly through memmap and check
  // (a) whether repeated reads of the same bytes agree with each other and
  // (b) whether they match the written pattern. this splits a flaky read
  // (passes disagree -> physical bus/power/ground) from statically-wrong data
  // (passes agree but != pat -> the program/erase side, not the read)
  {
    uint8_t *a = rbuf;       // pass A bytes [0..255]
    uint8_t *b = rbuf + 256; // pass B bytes [256..511]
    const uint32_t probe_off = BENCH_WRITE_OFFSET;
    uint32_t unstable = 0, wrong = 0;
    uint8_t err_mask = 0; // OR of (want ^ got): which byte-bit positions ever wrong
    for (uint32_t pass = 0; pass < 16U; pass++) {
      qspi_memmap_invalidate((const void *)(flash + probe_off), PAGE_SIZE);
      for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        a[i] = flash[probe_off + i];
      }
      qspi_memmap_invalidate((const void *)(flash + probe_off), PAGE_SIZE);
      for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        b[i] = flash[probe_off + i];
      }
      for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        if (a[i] != b[i]) {
          unstable++;
        }
        if (a[i] != pat[i]) {
          wrong++;
          err_mask |= (uint8_t)(a[i] ^ pat[i]);
        }
      }
    }
    // in quad output each byte is two nibbles, bit n of a nibble comes off IOn.
    // so byte-bits {0,4}->IO0, {1,5}->IO1, {2,6}->IO2, {3,7}->IO3. a bad data
    // line shows up as a fixed pair of bits in the error mask
    bsp_printf("memmap probe @ +0x%06lX, 16 passes: %lu unstable, %lu wrong vs pat\n",
               (unsigned long)probe_off, (unsigned long)unstable, (unsigned long)wrong);
    bsp_printf("  err bitmask 0x%02X  IO0:%c IO1:%c IO2:%c IO3:%c\n", err_mask,
               (err_mask & 0x11) ? 'X' : '.', (err_mask & 0x22) ? 'X' : '.',
               (err_mask & 0x44) ? 'X' : '.', (err_mask & 0x88) ? 'X' : '.');
  }

  // cross-check: verify the SAME write region through the memmap engine
  bool mm_ok = true;
  qspi_memmap_invalidate((const void *)(flash + BENCH_WRITE_OFFSET), BENCH_WRITE_BYTES);
  for (uint32_t off = 0; off < BENCH_WRITE_BYTES; off++) {
    uint8_t got = flash[BENCH_WRITE_OFFSET + off];
    if (got != pat[off & (PAGE_SIZE - 1)]) {
      bsp_printf("verify memmap: MISMATCH @ +0x%06lX want 0x%02X got 0x%02X\n",
                 (unsigned long)(BENCH_WRITE_OFFSET + off),
                 pat[off & (PAGE_SIZE - 1)], got);
      mm_ok = false;
      break;
    }
  }
  bsp_printf("verify memmap: %s\n", mm_ok ? "OK" : "MISMATCH");

  // mmap read bandwidth (invalidate first so reads come from flash, not cache)
  qspi_memmap_invalidate((const void *)flash, BENCH_MMAP_READ_BYTES);
  const volatile uint32_t *p = (const volatile uint32_t *)flash;
  uint32_t words = BENCH_MMAP_READ_BYTES / 4U;
  sum = 0;
  t0 = chVTGetSystemTimeX();
  for (uint32_t i = 0; i < words; i++) {
    sum += p[i];
  }
  uint32_t mm_us = elapsed_us(t0, chVTGetSystemTimeX());
  g_sink = sum;
  bsp_printf("read memmap (dual): %lu KB in %lu us = %lu KB/s\n",
             (unsigned long)(BENCH_MMAP_READ_BYTES / 1024U), (unsigned long)mm_us,
             (unsigned long)kbps(BENCH_MMAP_READ_BYTES, mm_us));

  qspi_memmap_disable(&qspi_cfg);

  bsp_printf("\n--- test_qspi_bench finished ---\r\n");
  while (1) {
    chThdSleepMilliseconds(1000);
  }
}
