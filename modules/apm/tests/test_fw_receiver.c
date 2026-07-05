// framed update receiver on USART3 (PB10/PB11). exercises the lib/bootloader
// transport: HELLO handshake, MANIFEST, streamed DATA frames with a running
// image crc, then a RESULT verdict on DONE. validates every frame's crc and the
// whole-image crc against the manifest. this is the receive path the real
// bootloader grows from - here it validates and discards (no flash write yet),
// so it isolates protocol correctness from flash speed.
//
// data + control frames on USART3; human-readable status on the UART5 console.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "bootloader/protocol.h"
#include "bootloader/frame.h"
#include "bootloader/crc32.h"

#include <string.h>

// match the host tool
#define RX_BAUD 2000000

static const SerialConfig uart3_cfg = {
  .speed = RX_BAUD,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

static bl_frame_rx rx;
static uint8_t txbuf[8U + BL_MAX_PAYLOAD + 4U];
static uint8_t rdbuf[512];

// session state
static bl_manifest manifest;
static uint32_t img_crc;
static uint32_t img_bytes;
static uint16_t exp_seq;
static int have_manifest;

static void send_frame(uint8_t type, uint16_t seq, const void *pl, uint16_t len) {
  size_t n = bl_frame_encode(type, seq, pl, len, txbuf, sizeof(txbuf));
  if (n > 0U) {
    sdWrite(&SD3, txbuf, n);
  }
}

// the APM receiver accepts its own stm32 image plus fpga bitstreams (routed to
// qspi); reject anything else
static int target_ok(uint16_t t) {
  return t == BL_TARGET_APM_H755 ||
         t == BL_TARGET_FPGA_GW2AR18 ||
         t == BL_TARGET_FPGA_GW5A25;
}

static void handle_frame(const bl_frame *f) {
  switch (f->type) {
    case BL_HELLO: {
      have_manifest = 0;
      img_bytes = 0U;
      exp_seq = 0U;
      bl_hello h = {.version = BL_PROTO_VERSION, .max_payload = BL_MAX_PAYLOAD};
      send_frame(BL_HELLO_ACK, 0U, &h, sizeof(h));
      bsp_printf("HELLO -> ack v%u maxpl %u\r\n", BL_PROTO_VERSION,
                 (unsigned)BL_MAX_PAYLOAD);
      break;
    }

    case BL_MANIFEST: {
      if (f->len < sizeof(bl_manifest)) {
        bl_result r = {.status = BL_ERR_PROTO, .reserved = 0, .bytes = 0};
        send_frame(BL_NAK, 0U, &r, sizeof(r));
        break;
      }
      memcpy(&manifest, f->payload, sizeof(manifest));
      if (manifest.magic != BL_MANIFEST_MAGIC || !target_ok(manifest.target)) {
        bl_result r = {.status = BL_ERR_TARGET, .reserved = 0, .bytes = 0};
        send_frame(BL_NAK, 0U, &r, sizeof(r));
        bsp_printf("MANIFEST rejected: magic=0x%08lX target=%u\r\n",
                   (unsigned long)manifest.magic, (unsigned)manifest.target);
        break;
      }
      img_crc = BL_CRC32_INIT;
      img_bytes = 0U;
      exp_seq = 0U;
      have_manifest = 1;
      bsp_printf("MANIFEST target=%u len=%lu crc=0x%08lX\r\n",
                 (unsigned)manifest.target, (unsigned long)manifest.length,
                 (unsigned long)manifest.crc32);
      break;
    }

    case BL_DATA: {
      if (!have_manifest) {
        break;
      }
      img_crc = bl_crc32_update(img_crc, f->payload, f->len);
      img_bytes += f->len;
      exp_seq++;
      break;
    }

    case BL_DONE: {
      uint16_t st;
      uint32_t final_crc = bl_crc32_final(img_crc);
      if (!have_manifest) {
        st = BL_ERR_PROTO;
      } else if (img_bytes != manifest.length) {
        st = BL_ERR_SIZE;
      } else if (final_crc != manifest.crc32) {
        st = BL_ERR_CRC;
      } else {
        st = BL_OK;
      }
      bl_result r = {.status = st, .reserved = 0, .bytes = img_bytes};
      send_frame(BL_RESULT, 0U, &r, sizeof(r));
      bsp_printf("DONE bytes=%lu crc=0x%08lX (want %lu / 0x%08lX) -> %s\r\n",
                 (unsigned long)img_bytes, (unsigned long)final_crc,
                 (unsigned long)manifest.length, (unsigned long)manifest.crc32,
                 (st == BL_OK) ? "OK" : "FAIL");
      break;
    }

    default:
      break;
  }
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fw_receiver ---\r\n");

  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(7)); // USART3_TX
  palSetPadMode(GPIOB, 11, PAL_MODE_ALTERNATE(7)); // USART3_RX
  sdStart(&SD3, &uart3_cfg);
  bl_frame_rx_init(&rx);

  bsp_printf("fw receiver ready @ %u baud\r\n", (unsigned)RX_BAUD);

  for (;;) {
    size_t n = sdReadTimeout(&SD3, rdbuf, sizeof(rdbuf), TIME_MS2I(100));
    for (size_t i = 0; i < n; i++) {
      int s = bl_frame_feed(&rx, rdbuf[i]);
      if (s == BL_FRAME_OK) {
        handle_frame(&rx.frame);
      } else if (s == BL_FRAME_BAD_CRC) {
        bl_result r = {.status = BL_ERR_CRC, .reserved = 0, .bytes = img_bytes};
        send_frame(BL_NAK, 0U, &r, sizeof(r));
        bsp_printf("frame crc bad\r\n");
      }
    }
  }
}
