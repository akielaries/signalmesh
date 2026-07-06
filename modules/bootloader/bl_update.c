// UART4 firmware-update receiver for the bootloader. reuses the proven UART4
// async-DMA + handshake pattern from test_uart4_rx_bench.c (the sync uart api is
// broken on this H7 driver, and we must never uartStop in a loop), then parses
// the framed protocol (MANIFEST/DATA/DONE), buffers the image in RAM, verifies
// the transfer crc, and writes it to the target QSPI slot in indirect mode.
//
// QSPI is left in indirect mode by qspi bringup, so erase/program work directly;
// bl_main enables memory-mapped mode afterwards for the boot cascade.

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/qspi_memmap.h"

#include "bootloader/protocol.h"
#include "bootloader/frame.h"
#include "bootloader/crc32.h"
#include "bootloader/image.h"
#include "bootloader/memmap.h"

#include "bl_update.h"

#define UPD_BAUD       1000000        // match on the host (--baud 1000000)
#define RXSZ           2048U          // DMA chunk (32-byte aligned multiple)
#define UPD_MAX_IMAGE  (256U * 1024U) // POC image cap (RAM-buffered)
#define QSPI_SECTOR    4096U
#define QSPI_PAGE      256U

// --- UART4 DMA plumbing (from test_uart4_rx_bench.c) ---
CC_ALIGN_DATA(32) static uint8_t rxbuf[2][RXSZ];
static volatile uint8_t cur;
static mailbox_t rx_mb;
static msg_t rx_mb_buf[16];
static volatile int phase; // 0 = handshake, 1 = stream

CC_ALIGN_DATA(32) static uint8_t hs_rx[32];
CC_ALIGN_DATA(32) static uint8_t hs_tx[32]; // outbound frames (ack/result/nak)
static volatile bool hs_rx_done;
static volatile bool hs_tx_done;

// --- session state ---
static const qspi_memmap_config_t *g_qspi;
static bl_frame_rx g_rx;
static bl_manifest g_manifest;
static int g_have_manifest;
static uint32_t g_dst_off; // device offset of the target slot
static uint32_t g_recv;    // bytes buffered
static uint16_t g_status;  // final status

static uint8_t g_img[UPD_MAX_IMAGE]; // RAM buffer for the incoming image

// ---- DMA callbacks ----
static void dispatch(UARTDriver *uartp, size_t got) {
  uint8_t done = cur;
  cur ^= 1U;
  uartStartReceiveI(uartp, RXSZ, rxbuf[cur]);
  if (got > 0U) {
    (void)chMBPostI(&rx_mb, (msg_t)(((uint32_t)done << 16) | (uint32_t)got));
  }
}

static void rxend_cb(UARTDriver *uartp) {
  if (phase == 0) {
    hs_rx_done = true;
    return;
  }
  chSysLockFromISR();
  dispatch(uartp, RXSZ);
  chSysUnlockFromISR();
}

static void idle_cb(UARTDriver *uartp) {
  if (phase == 0) {
    return;
  }
  chSysLockFromISR();
  size_t rem = uartStopReceiveI(uartp);
  if (rem != UART_ERR_NOT_ACTIVE) {
    dispatch(uartp, RXSZ - rem);
  }
  chSysUnlockFromISR();
}

static void txend_cb(UARTDriver *uartp) {
  (void)uartp;
  hs_tx_done = true;
}

static void err_cb(UARTDriver *uartp, uartflags_t e) {
  (void)uartp;
  (void)e;
}

static const UARTConfig uart_cfg = {
  .txend1_cb  = txend_cb,
  .txend2_cb  = NULL,
  .rxend_cb   = rxend_cb,
  .rxchar_cb  = NULL,
  .rxerr_cb   = err_cb,
  .timeout_cb = idle_cb,
  .timeout    = 0,
  .speed      = UPD_BAUD,
  .cr1        = USART_CR1_IDLEIE,
  .cr2        = USART_CR2_STOP1_BITS,
  .cr3        = 0,
};

// blocking send of a framed reply (small frames fit in the aligned hs_tx buffer)
static void send_reply(uint8_t type, const void *pl, uint16_t len) {
  size_t n = bl_frame_encode(type, 0, pl, len, hs_tx, sizeof(hs_tx));
  if (n == 0U) {
    return;
  }
  cacheBufferFlush(hs_tx, n);
  hs_tx_done = false;
  uartStartSend(&UARTD4, n, hs_tx);
  while (!hs_tx_done) {
    chThdSleepMilliseconds(1);
  }
  chThdSleepMilliseconds(2);
}

// erase + program the image into the slot (indirect mode). returns 0 on success
static int write_slot(uint32_t dev_off, const uint8_t *data, uint32_t len) {
  for (uint32_t o = 0; o < len; o += QSPI_SECTOR) {
    if (!qspi_memmap_erase_sector(g_qspi, dev_off + o)) {
      return -1;
    }
  }
  for (uint32_t o = 0; o < len; o += QSPI_PAGE) {
    uint32_t chunk = (len - o) < QSPI_PAGE ? (len - o) : QSPI_PAGE;
    if (!qspi_memmap_program(g_qspi, dev_off + o, data + o, chunk)) {
      return -1;
    }
  }
  return 0;
}

// handle one complete frame; returns 1 when the session is finished
static int handle_frame(const bl_frame *f) {
  switch (f->type) {
    case BL_MANIFEST: {
      if (f->len < sizeof(bl_manifest)) {
        g_status = BL_ERR_PROTO;
        bl_result r = {.status = BL_ERR_PROTO, .reserved = 0, .bytes = 0};
        send_reply(BL_NAK, &r, sizeof(r));
        return 1;
      }
      memcpy(&g_manifest, f->payload, sizeof(g_manifest));

      uint32_t slot_base;
      switch (g_manifest.target) {
        case BL_TARGET_APM_H755:
          slot_base = bl_memmap[BL_SLOT_APP_1].base; // active app slot
          break;
        case BL_TARGET_FPGA_GW2AR18:
        case BL_TARGET_FPGA_GW5A25:
          slot_base = bl_memmap[BL_SLOT_FPGA_ACTIVE].base;
          break;
        default:
          g_status = BL_ERR_TARGET;
          {
            bl_result r = {.status = BL_ERR_TARGET, .reserved = 0, .bytes = 0};
            send_reply(BL_NAK, &r, sizeof(r));
          }
          return 1;
      }

      if (g_manifest.magic != BL_MANIFEST_MAGIC ||
          g_manifest.length > UPD_MAX_IMAGE) {
        g_status = (g_manifest.length > UPD_MAX_IMAGE) ? BL_ERR_SIZE : BL_ERR_PROTO;
        bl_result r = {.status = g_status, .reserved = 0, .bytes = 0};
        send_reply(BL_NAK, &r, sizeof(r));
        return 1;
      }

      g_dst_off = slot_base - g_qspi->base;
      g_recv = 0;
      g_have_manifest = 1;
      bsp_printf("update: manifest target=%u len=%lu -> %s\r\n",
                 (unsigned)g_manifest.target, (unsigned long)g_manifest.length,
                 (g_manifest.target == BL_TARGET_APM_H755) ? "app_1" : "fpga_active");
      return 0;
    }

    case BL_DATA: {
      if (!g_have_manifest) {
        return 0;
      }
      if (g_recv + f->len > UPD_MAX_IMAGE) {
        g_status = BL_ERR_SIZE;
        return 0;
      }
      memcpy(g_img + g_recv, f->payload, f->len);
      g_recv += f->len;
      return 0;
    }

    case BL_DONE: {
      uint16_t st;
      if (!g_have_manifest) {
        st = BL_ERR_PROTO;
      } else if (g_recv != g_manifest.length) {
        st = BL_ERR_SIZE;
      } else if (bl_crc32(g_img, g_recv) != g_manifest.crc32) {
        st = BL_ERR_CRC;
      } else {
        bsp_printf("update: crc ok, writing %lu bytes to slot...\r\n",
                   (unsigned long)g_recv);
        st = (write_slot(g_dst_off, g_img, g_recv) == 0) ? BL_OK : BL_ERR_FLASH;
      }
      g_status = st;
      bl_result r = {.status = st, .reserved = 0, .bytes = g_recv};
      send_reply(BL_RESULT, &r, sizeof(r));
      return 1;
    }

    default:
      return 0;
  }
}

// wait up to window_ms for a HELLO frame. returns 1 if a HELLO arrived
static int wait_hello(uint32_t window_ms) {
  const size_t hlen = 8U + sizeof(bl_hello) + 4U;
  phase = 0;
  hs_rx_done = false;
  uartStartReceive(&UARTD4, hlen, hs_rx);

  uint32_t waited = 0;
  while (!hs_rx_done && waited < window_ms) {
    chThdSleepMilliseconds(5);
    waited += 5;
  }
  if (!hs_rx_done) {
    uartStopReceive(&UARTD4);
    return 0;
  }

  cacheBufferInvalidate(hs_rx, hlen);
  bl_frame_rx rx;
  bl_frame_rx_init(&rx);
  for (size_t i = 0; i < hlen; i++) {
    if (bl_frame_feed(&rx, hs_rx[i]) == BL_FRAME_OK && rx.frame.type == BL_HELLO) {
      return 1;
    }
  }
  return 0;
}

int bl_update_run(const qspi_memmap_config_t *qspi, uint32_t window_ms) {
  g_qspi = qspi;

  palSetPadMode(GPIOC, 11, PAL_MODE_ALTERNATE(8)); // UART4_RX
  palSetPadMode(GPIOC, 10, PAL_MODE_ALTERNATE(8)); // UART4_TX

  chMBObjectInit(&rx_mb, rx_mb_buf, 16);
  uartStart(&UARTD4, &uart_cfg);

  bsp_printf("update: listening on UART4 @ %u for %lums...\r\n",
             (unsigned)UPD_BAUD, (unsigned long)window_ms);
  if (!wait_hello(window_ms)) {
    bsp_printf("update: no host, proceeding to boot\r\n");
    return 0; // NOT uartStop (frees DMA -> rcc assert); leave the driver up
  }

  // reply HELLO_ACK while RX is idle. concurrent TX while a receive is armed is
  // unreliable on this H7 uart driver (the bench always acked when idle); the
  // host waits ~20ms after the ack before streaming, so arming the receiver
  // right after is in time.
  bl_hello h = {.version = BL_PROTO_VERSION, .max_payload = BL_MAX_PAYLOAD};
  send_reply(BL_HELLO_ACK, &h, sizeof(h));

  // now arm the stream receiver
  bl_frame_rx_init(&g_rx);
  g_have_manifest = 0;
  g_recv = 0;
  g_status = BL_ERR_PROTO;
  msg_t junk;
  while (chMBFetchTimeout(&rx_mb, &junk, TIME_IMMEDIATE) == MSG_OK) {
  }
  cur = 0;
  phase = 1;
  uartStartReceive(&UARTD4, RXSZ, rxbuf[cur]);
  bsp_printf("update: host connected, receiving...\r\n");

  // consume chunks, feed the frame parser, until DONE or a stall
  for (;;) {
    msg_t m;
    if (chMBFetchTimeout(&rx_mb, &m, TIME_MS2I(5000)) != MSG_OK) {
      bsp_printf("update: stalled, aborting\r\n");
      break;
    }
    uint8_t idx = (uint8_t)(((uint32_t)m >> 16) & 1U);
    size_t len = (size_t)((uint32_t)m & 0xFFFFU);
    cacheBufferInvalidate(rxbuf[idx], RXSZ);

    int fin = 0;
    for (size_t i = 0; i < len; i++) {
      if (bl_frame_feed(&g_rx, rxbuf[idx][i]) == BL_FRAME_OK) {
        if (handle_frame(&g_rx.frame)) {
          fin = 1;
          break;
        }
      }
    }
    if (fin) {
      break;
    }
  }

  uartStopReceive(&UARTD4);
  bsp_printf("update: done, status=%u, %lu bytes\r\n", (unsigned)g_status,
             (unsigned long)g_recv);
  return 1;
}
