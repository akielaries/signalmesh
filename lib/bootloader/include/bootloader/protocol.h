// signalmesh firmware-update wire protocol - shared by the STM32 receiver and
// the x86 host tool (tools/fw_update). this is the single source of truth for
// the frame layout and manifest; both sides include this exact file so the
// format cannot drift.
//
// c and c++ compatible.

#ifndef BOOTLOADER_PROTOCOL_H
#define BOOTLOADER_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// frame start-of-frame magic (little-endian on the wire: A5 5A)
#define BL_SOF          0x5AA5U

// largest payload a single frame carries
#define BL_MAX_PAYLOAD  1024U

// protocol version, bumped on any breaking change to this header
#define BL_PROTO_VERSION 1U

// frame types
enum bl_frame_type {
  BL_HELLO      = 0x01, // host->mcu: begin session (payload: bl_hello)
  BL_HELLO_ACK  = 0x02, // mcu->host: session accepted (payload: bl_hello)
  BL_MANIFEST   = 0x03, // host->mcu: image descriptor (payload: bl_manifest)
  BL_DATA       = 0x04, // host->mcu: image chunk (payload: raw bytes)
  BL_DONE       = 0x05, // host->mcu: all chunks sent
  BL_RESULT     = 0x06, // mcu->host: final verdict (payload: bl_result)
  BL_NAK        = 0x07, // mcu->host: something failed mid-stream (payload: bl_result)
  BL_ABORT      = 0x08, // either way: tear down the session
};

// which component an image targets - the mcu refuses a mismatched target
enum bl_target {
  BL_TARGET_NONE        = 0,
  BL_TARGET_APM_H755    = 1, // stm32 application image (internal flash A/B)
  BL_TARGET_FPGA_GW2AR18 = 2, // tang nano 20k bitstream (.bin, qspi)
  BL_TARGET_FPGA_GW5A25  = 3, // tang primer 25k bitstream (.bin, qspi)
};

// result / nak reason codes
enum bl_status {
  BL_OK             = 0,
  BL_ERR_CRC        = 1, // a frame or the whole-image crc failed
  BL_ERR_TARGET     = 2, // manifest target not supported on this board
  BL_ERR_SIZE       = 3, // image larger than the destination slot
  BL_ERR_SEQ        = 4, // out-of-order / missing frame
  BL_ERR_PROTO      = 5, // malformed frame or bad protocol version
  BL_ERR_FLASH      = 6, // write/erase/verify of the destination failed
};

// frame header as it appears on the wire, immediately followed by payload[len]
// then a u32 crc32. packed so the layout is identical on both sides.
#pragma pack(push, 1)
typedef struct {
  uint16_t sof;   // BL_SOF
  uint8_t  type;  // enum bl_frame_type
  uint8_t  flags; // reserved, 0
  uint16_t seq;   // frame sequence, starts at 0
  uint16_t len;   // payload length, 0..BL_MAX_PAYLOAD
} bl_frame_hdr;

// BL_HELLO / BL_HELLO_ACK payload
typedef struct {
  uint16_t version;   // BL_PROTO_VERSION
  uint16_t max_payload;
} bl_hello;

// BL_MANIFEST payload - describes the image that the following DATA frames carry
typedef struct {
  uint32_t magic;      // BL_SOF-independent manifest magic (see mkupdate)
  uint16_t target;     // enum bl_target
  uint16_t reserved;
  uint32_t version;    // image version (component-defined)
  uint32_t length;     // total image byte count
  uint32_t crc32;      // crc32 over the whole image
} bl_manifest;

// BL_RESULT / BL_NAK payload
typedef struct {
  uint16_t status;     // enum bl_status
  uint16_t reserved;
  uint32_t bytes;      // bytes accepted so far
} bl_result;
#pragma pack(pop)

// manifest magic (spells "SMUP" - signalmesh update)
#define BL_MANIFEST_MAGIC 0x50554D53U

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_PROTOCOL_H
