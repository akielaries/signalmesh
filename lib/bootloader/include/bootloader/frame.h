// frame reassembly + encoding for the update protocol. feed received bytes one
// at a time to bl_frame_feed(); it resyncs on the SOF magic, validates the
// trailing crc32, and hands back a complete frame. bl_frame_encode() builds a
// frame for transmit. no dynamic allocation - one bl_frame_rx per link.

#ifndef BOOTLOADER_FRAME_H
#define BOOTLOADER_FRAME_H

#include <stddef.h>
#include <stdint.h>

#include "bootloader/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

enum bl_frame_status {
  BL_FRAME_MORE    = 0, // need more bytes
  BL_FRAME_OK      = 1, // a complete, crc-valid frame is in rx->frame
  BL_FRAME_BAD_CRC = 2, // frame assembled but crc mismatched (resynced)
  BL_FRAME_BAD_LEN = 3, // len field exceeded BL_MAX_PAYLOAD (resynced)
};

typedef struct {
  uint8_t  type;
  uint8_t  flags;
  uint16_t seq;
  uint16_t len;
  uint8_t  payload[BL_MAX_PAYLOAD];
} bl_frame;

typedef struct {
  int      state;
  uint32_t crc;       // running crc over type..payload
  uint8_t  hdr[6];    // type, flags, seq_lo, seq_hi, len_lo, len_hi
  uint8_t  hdr_idx;
  uint16_t pay_idx;
  uint8_t  crcbuf[4];
  uint8_t  crc_idx;
  bl_frame frame;     // valid when BL_FRAME_OK is returned
} bl_frame_rx;

void bl_frame_rx_init(bl_frame_rx *rx);

// feed one received byte; returns an enum bl_frame_status
int bl_frame_feed(bl_frame_rx *rx, uint8_t b);

// encode a frame into out (needs 8 + len + 4 bytes). returns total bytes
// written, or 0 if len is too big or cap is too small
size_t bl_frame_encode(uint8_t type, uint16_t seq, const void *payload,
                       uint16_t len, uint8_t *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_FRAME_H
