// host->STM32 bulk RX benchmark on USART3 (PB10 TX / PB11 RX).
// a linux sender blasts N raw bytes; this drains them, times first->last byte,
// and reports throughput + an FNV-1a checksum so the host can verify integrity.
// data comes in on USART3; all reporting goes out the UART5 debug console so the
// measured path is never touched by prints. loops for repeated runs.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

// match this on the host. drop to 1000000/921600 if the dongle can't do 2M
#define BENCH_BAUD    2000000
// drain buffer; large chunks keep the per-read overhead low
#define CHUNK         4096U
// idle gap that marks end-of-transfer (no byte for this long -> done)
#define IDLE_MS       250U
// progress tick on the console
#define MARK_BYTES    (256U * 1024U)

static const SerialConfig uart3_cfg = {
  .speed = BENCH_BAUD,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

static uint8_t rxbuf[CHUNK];

// fnv-1a 32-bit, must match the host implementation
static inline uint32_t fnv1a(uint32_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0U; i < n; i++) {
    h ^= p[i];
    h *= 0x01000193U;
  }
  return h;
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_uart3_rx_bench ---\r\n");

  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(7)); // USART3_TX
  palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7)); // USART3_RX
  sdStart(&SD3, &uart3_cfg);

  bsp_printf("RX bench ready @ %u baud, buf %u, idle %ums\r\n",
             (unsigned)BENCH_BAUD, (unsigned)CHUNK, (unsigned)IDLE_MS);

  for (;;) {
    bsp_printf("\r\nwaiting for transfer...\r\n");

    // block until the first byte, then start timing
    msg_t first = sdGet(&SD3);
    if (first < 0) {
      continue;
    }

    systime_t start = chVTGetSystemTimeX();
    systime_t last = start;
    uint32_t total = 1U;
    uint32_t next_mark = MARK_BYTES;
    uint8_t fb = (uint8_t)first;
    uint32_t sum = fnv1a(0x811C9DC5U, &fb, 1U);

    for (;;) {
      size_t n = sdReadTimeout(&SD3, rxbuf, CHUNK, TIME_MS2I(IDLE_MS));
      if (n == 0U) {
        break; // idle gap: transfer complete
      }
      total += (uint32_t)n;
      sum = fnv1a(sum, rxbuf, n);
      last = chVTGetSystemTimeX();

      if (total >= next_mark) {
        bsp_printf("  ...%lu KB\r\n", (unsigned long)(total / 1024U));
        next_mark += MARK_BYTES;
      }
    }

    uint32_t ms = (uint32_t)chTimeI2MS(chTimeDiffX(start, last));
    if (ms == 0U) {
      ms = 1U; // guard tiny transfers against divide-by-zero
    }

    float secs = (float)ms / 1000.0f;
    float kbps = ((float)total / 1024.0f) / secs;
    float mbits = ((float)total * 8.0f) / (secs * 1000000.0f);
    float line_bps = (float)BENCH_BAUD / 10.0f; // 8N1 = 10 bits/byte
    float util = ((float)total / secs) / line_bps * 100.0f;

    bsp_printf("--- transfer done ---\r\n");
    bsp_printf("  bytes    : %lu\r\n", (unsigned long)total);
    bsp_printf("  time     : %lu ms\r\n", (unsigned long)ms);
    bsp_printf("  rate     : %.1f KB/s  (%.2f Mbit/s)\r\n", kbps, mbits);
    bsp_printf("  line util: %.1f %%\r\n", util);
    bsp_printf("  fnv1a    : 0x%08lX\r\n", (unsigned long)sum);
  }
}
