// on-flash image header, written ahead of a stored image (app slot, golden,
// bitstream) so the bootloader can validate it independently of the
// transfer-time manifest. distinct from bl_manifest (protocol.h), which
// describes an in-flight transfer.
//
// slot layout (built by mkupdate): [bl_image_header][pad][app image]. the app
// image starts at BL_IMAGE_OFFSET into the slot - a VTOR-aligned boundary so the
// app can be executed in place (XIP) from the QSPI-mapped slot. all memory-
// mapped, so validation is plain pointer access; only writes are medium-specific.

#ifndef BOOTLOADER_IMAGE_H
#define BOOTLOADER_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BL_IMAGE_MAGIC  0x314D4C42U // 'BLM1'
#define BL_IMAGE_OFFSET 0x200U      // app image sits here in the slot (VTOR-aligned)

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;        // BL_IMAGE_MAGIC
  uint16_t target;       // enum bl_target (protocol.h)
  uint16_t flags;
  uint32_t version;
  uint32_t length;       // app image byte count (at BL_IMAGE_OFFSET in the slot)
  uint32_t image_crc32;  // crc32 over the app image bytes
  uint32_t header_crc32; // crc32 over this header with this field zeroed
} bl_image_header;
#pragma pack(pop)

// validate a stored image whose header sits at `base` (a memory-mapped address:
// internal flash 0x08.., or QSPI 0x90..). checks magic + header crc, then the
// app-image crc at base+BL_IMAGE_OFFSET. returns 0 if good, negative otherwise.
int bl_image_validate(const void *base);

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_IMAGE_H
