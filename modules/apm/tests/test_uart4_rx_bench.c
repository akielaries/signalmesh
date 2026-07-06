// host->STM32 bulk RX benchmark on UART4 (PC11 = UART4_RX, PC10 = UART4_TX, AF8)
// using the ChibiOS DMA UART driver (UARTD4). a framed HELLO/HELLO_ACK handshake
// runs first, then double-buffered DMA reception streams the bulk data; each
// chunk is hashed (fnv-1a) via a mailbox so the host can verify integrity.
//
// the driver is started ONCE and never uartStop()'d - stopping it frees the DMA
// streams and trips a ChibiOS rcc refcount assert. a single config serves both
// phases; a `phase` flag steers the rx/idle callbacks. only uartStartReceive /
// uartStopReceive are used to switch between phases (they don't touch clocks).
//
// DMA moves physical AXI SRAM with the M7 d-cache on, so buffers we read need a
// cache invalidate and buffers we send need a flush.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "bootloader/protocol.h"
#include "bootloader/frame.h"

// match this on the host (./update.bin --test --baud ...)
#define BENCH_BAUD  3000000
#define RXSZ        4096U             // DMA chunk, 32-byte aligned multiple
#define IDLE_END_MS 250U              // silence that marks end-of-transfer
#define MB_SENTINEL ((msg_t)0xFFFFFFFFU)
#define FNV_INIT    0x811C9DC5U


// benchmark double buffer + chunk mailbox (idx<<16 | length)
CC_ALIGN_DATA(32) static uint8_t rxbuf[2][RXSZ];
static volatile uint8_t cur;
static mailbox_t rx_mb;
static msg_t rx_mb_buf[16];

static virtual_timer_t end_vt;
static volatile systime_t t_start;
static volatile systime_t t_last;
static volatile bool started;

// handshake buffers + completion flags, cache-line aligned for DMA
CC_ALIGN_DATA(32) static uint8_t hs_rx[32];
CC_ALIGN_DATA(32) static uint8_t hs_tx[32];
static volatile bool hs_rx_done;
static volatile bool hs_tx_done;

static volatile int phase; // 0 = handshake, 1 = benchmark

static uint32_t fnv1a(uint32_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; i++) {
    h ^= p[i];
    h *= 0x01000193U;
  }
  return h;
}

// 250ms with no new data -> tell the main loop the transfer is done (VT context,
// system already locked)
static void end_cb(virtual_timer_t *vtp, void *p) {
  (void)vtp;
  (void)p;
  (void)chMBPostI(&rx_mb, MB_SENTINEL);
}

// account for a received chunk and keep the DMA rolling into the other buffer
static void dispatch(UARTDriver *uartp, size_t got) {
  uint8_t done = cur;
  cur ^= 1U;
  uartStartReceiveI(uartp, RXSZ, rxbuf[cur]);
  if (got > 0U) {
    if (!started) {
      started = true;
      t_start = chVTGetSystemTimeX();
    }
    t_last = chVTGetSystemTimeX();
    (void)chMBPostI(&rx_mb, (msg_t)(((uint32_t)done << 16) | (uint32_t)got));
    if (chVTIsArmedI(&end_vt)) {
      chVTResetI(&end_vt);
    }
    chVTSetI(&end_vt, TIME_MS2I(IDLE_END_MS), end_cb, NULL);
  }
}

// rx buffer full: handshake -> signal, benchmark -> dispatch a chunk
static void rxend_cb(UARTDriver *uartp) {
  if (phase == 0) {
    hs_rx_done = true;
  } else {
    chSysLockFromISR();
    dispatch(uartp, RXSZ);
    chSysUnlockFromISR();
  }
}

// idle line (benchmark only): end of a burst before the buffer filled
static void idle_cb(UARTDriver *uartp) {
  if (phase == 1) {
    chSysLockFromISR();
    size_t remaining = uartStopReceiveI(uartp);
    if (remaining != UART_ERR_NOT_ACTIVE) {
      dispatch(uartp, RXSZ - remaining);
    }
    chSysUnlockFromISR();
  }
}

// txend1 = DMA transfer complete (reliable); txend2/TC is flaky on this driver
static void txend_cb(UARTDriver *uartp) {
  (void)uartp;
  hs_tx_done = true;
}

static void err_cb(UARTDriver *uartp, uartflags_t e) {
  (void)uartp;
  (void)e; // framing/overrun auto-cleared, keep going
}

static const UARTConfig uart_cfg = {
  .txend1_cb  = txend_cb,
  .txend2_cb  = NULL,
  .rxend_cb   = rxend_cb,
  .rxchar_cb  = NULL,
  .rxerr_cb   = err_cb,
  .timeout_cb = idle_cb,
  .timeout    = 0,                 // 0 + IDLEIE => idle-line interrupt
  .speed      = BENCH_BAUD,
  .cr1        = USART_CR1_IDLEIE,
  .cr2        = USART_CR2_STOP1_BITS,
  .cr3        = 0,
};

// receive one HELLO-sized frame via DMA and check it is a HELLO
static bool recv_hello(void) {
  const size_t hello_len = 8U + sizeof(bl_hello) + 4U;
  hs_rx_done = false;
  uartStartReceive(&UARTD4, hello_len, hs_rx);
  while (!hs_rx_done) {
    chThdSleepMilliseconds(2);
  }
  cacheBufferInvalidate(hs_rx, hello_len);

  bl_frame_rx rx;
  bl_frame_rx_init(&rx);
  for (size_t i = 0; i < hello_len; i++) {
    if (bl_frame_feed(&rx, hs_rx[i]) == BL_FRAME_OK && rx.frame.type == BL_HELLO) {
      return true;
    }
  }
  return false;
}

static void send_ack(void) {
  bl_hello h;
  h.version = BL_PROTO_VERSION;
  h.max_payload = BL_MAX_PAYLOAD;
  size_t n = bl_frame_encode(BL_HELLO_ACK, 0, &h, sizeof(h), hs_tx, sizeof(hs_tx));
  if (n == 0U) {
    return;
  }
  cacheBufferFlush(hs_tx, n);
  hs_tx_done = false;
  uartStartSend(&UARTD4, n, hs_tx);
  while (!hs_tx_done) {
    chThdSleepMilliseconds(1);
  }
  chThdSleepMilliseconds(2); // let the last bits shift out
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_uart4_rx_bench: UART4 DMA RX + handshake ---\r\n");

  // PC11 = UART4_RX, PC10 = UART4_TX (AF8)
  palSetPadMode(GPIOC, 11, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOC, 10, PAL_MODE_ALTERNATE(8));

  chVTObjectInit(&end_vt);
  chMBObjectInit(&rx_mb, rx_mb_buf, 16);
  uartStart(&UARTD4, &uart_cfg); // once - never uartStop (frees DMA -> rcc assert)
  bsp_printf("UART4 @ %u baud, RX PC11 / TX PC10\r\n", (unsigned)BENCH_BAUD);

  for (;;) {
    // handshake: wait for HELLO, reply HELLO_ACK
    phase = 0;
    bsp_printf("\r\nwaiting for handshake (HELLO)...\r\n");
    while (!recv_hello()) {
      // keep receiving frames until a valid HELLO arrives
    }
    bsp_printf("HELLO received - replying HELLO_ACK\r\n");
    send_ack();

    // drain any stale mailbox entries from a previous run
    msg_t junk;
    while (chMBFetchTimeout(&rx_mb, &junk, TIME_IMMEDIATE) == MSG_OK) {
    }

    // benchmark: arm reception (host waits briefly after the ack)
    started = false;
    cur = 0U;
    uint32_t total = 0U;
    uint32_t fnv = FNV_INIT;
    phase = 1;
    uartStartReceive(&UARTD4, RXSZ, rxbuf[cur]);
    bsp_printf("link established - benchmarking...\r\n");

    // consume chunks until the idle-timer posts the sentinel
    for (;;) {
      msg_t m;
      if (chMBFetchTimeout(&rx_mb, &m, TIME_INFINITE) != MSG_OK) {
        continue;
      }
      if (m == MB_SENTINEL) {
        break;
      }
      uint8_t idx = (uint8_t)(((uint32_t)m >> 16) & 1U);
      size_t len = (size_t)((uint32_t)m & 0xFFFFU);
      cacheBufferInvalidate(rxbuf[idx], RXSZ);
      fnv = fnv1a(fnv, rxbuf[idx], len);
      total += (uint32_t)len;
    }

    uartStopReceive(&UARTD4); // stop the pending receive; keep the driver started

    uint32_t ms = (uint32_t)chTimeI2MS(chTimeDiffX(t_start, t_last));
    if (ms == 0U) {
      ms = 1U;
    }
    float secs = (float)ms / 1000.0f;
    float kbps = ((float)total / 1024.0f) / secs;
    float mbits = ((float)total * 8.0f) / (secs * 1000000.0f);
    float util = ((float)total / secs) / ((float)BENCH_BAUD / 10.0f) * 100.0f;

    bsp_printf("--- transfer done ---\r\n");
    bsp_printf("  bytes    : %lu\r\n", (unsigned long)total);
    bsp_printf("  time     : %lu ms\r\n", (unsigned long)ms);
    bsp_printf("  rate     : %.1f KB/s  (%.2f Mbit/s)\r\n", kbps, mbits);
    bsp_printf("  line util: %.1f %%\r\n", util);
    bsp_printf("  fnv1a    : 0x%08lX\r\n", (unsigned long)fnv);
  }
}
