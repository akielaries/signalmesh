#include "bootloader/frame.h"
#include "bootloader/crc32.h"

#include <string.h>

// parser states
enum { S_SOF1, S_SOF2, S_HDR, S_PAYLOAD, S_CRC };

// sof is BL_SOF (0x5AA5), little-endian on the wire: 0xA5 then 0x5A
#define SOF_LO 0xA5U
#define SOF_HI 0x5AU

void bl_frame_rx_init(bl_frame_rx *rx) {
  memset(rx, 0, sizeof(*rx));
  rx->state = S_SOF1;
}

static void resync(bl_frame_rx *rx) {
  rx->state = S_SOF1;
  rx->hdr_idx = 0;
  rx->pay_idx = 0;
  rx->crc_idx = 0;
}

int bl_frame_feed(bl_frame_rx *rx, uint8_t b) {
  switch (rx->state) {
    case S_SOF1:
      if (b == SOF_LO) {
        rx->state = S_SOF2;
      }
      return BL_FRAME_MORE;

    case S_SOF2:
      if (b == SOF_HI) {
        // start of a frame: crc covers everything from here (type..payload)
        rx->crc = BL_CRC32_INIT;
        rx->hdr_idx = 0;
        rx->state = S_HDR;
      } else if (b == SOF_LO) {
        // could be the real low byte after a stray 0xA5; stay armed
        rx->state = S_SOF2;
      } else {
        rx->state = S_SOF1;
      }
      return BL_FRAME_MORE;

    case S_HDR:
      rx->hdr[rx->hdr_idx++] = b;
      rx->crc = bl_crc32_update(rx->crc, &b, 1);
      if (rx->hdr_idx == 6U) {
        rx->frame.type  = rx->hdr[0];
        rx->frame.flags = rx->hdr[1];
        rx->frame.seq   = (uint16_t)(rx->hdr[2] | (rx->hdr[3] << 8));
        rx->frame.len   = (uint16_t)(rx->hdr[4] | (rx->hdr[5] << 8));
        if (rx->frame.len > BL_MAX_PAYLOAD) {
          resync(rx);
          return BL_FRAME_BAD_LEN;
        }
        rx->pay_idx = 0;
        rx->state = (rx->frame.len == 0U) ? S_CRC : S_PAYLOAD;
      }
      return BL_FRAME_MORE;

    case S_PAYLOAD:
      rx->frame.payload[rx->pay_idx++] = b;
      rx->crc = bl_crc32_update(rx->crc, &b, 1);
      if (rx->pay_idx == rx->frame.len) {
        rx->crc_idx = 0;
        rx->state = S_CRC;
      }
      return BL_FRAME_MORE;

    case S_CRC:
      rx->crcbuf[rx->crc_idx++] = b;
      if (rx->crc_idx == 4U) {
        uint32_t got = (uint32_t)rx->crcbuf[0] |
                       ((uint32_t)rx->crcbuf[1] << 8) |
                       ((uint32_t)rx->crcbuf[2] << 16) |
                       ((uint32_t)rx->crcbuf[3] << 24);
        uint32_t calc = bl_crc32_final(rx->crc);
        resync(rx);
        return (got == calc) ? BL_FRAME_OK : BL_FRAME_BAD_CRC;
      }
      return BL_FRAME_MORE;

    default:
      resync(rx);
      return BL_FRAME_MORE;
  }
}

size_t bl_frame_encode(uint8_t type, uint16_t seq, const void *payload,
                       uint16_t len, uint8_t *out, size_t cap) {
  size_t need = 8U + (size_t)len + 4U;
  if (len > BL_MAX_PAYLOAD || cap < need) {
    return 0;
  }

  out[0] = SOF_LO;
  out[1] = SOF_HI;
  out[2] = type;
  out[3] = 0; // flags
  out[4] = (uint8_t)(seq & 0xFFU);
  out[5] = (uint8_t)(seq >> 8);
  out[6] = (uint8_t)(len & 0xFFU);
  out[7] = (uint8_t)(len >> 8);
  if (len > 0U && payload != NULL) {
    memcpy(out + 8, payload, len);
  }

  // crc32 over type..payload (everything after the sof)
  uint32_t crc = bl_crc32(out + 2, 6U + (size_t)len);
  size_t c = 8U + (size_t)len;
  out[c + 0] = (uint8_t)(crc & 0xFFU);
  out[c + 1] = (uint8_t)(crc >> 8);
  out[c + 2] = (uint8_t)(crc >> 16);
  out[c + 3] = (uint8_t)(crc >> 24);
  return need;
}
