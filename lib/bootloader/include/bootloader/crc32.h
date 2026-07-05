// crc32 (ieee 802.3, reflected, poly 0xEDB88320) - shared by firmware and host.
// use bl_crc32() for a one-shot buffer, or seed/update/finalize to run it over a
// stream of frames without buffering the whole image.

#ifndef BOOTLOADER_CRC32_H
#define BOOTLOADER_CRC32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// one-shot crc32 over a buffer
uint32_t bl_crc32(const void *data, size_t len);

// streaming variant: start with BL_CRC32_INIT, feed chunks, then finalize
#define BL_CRC32_INIT 0xFFFFFFFFU
uint32_t bl_crc32_update(uint32_t crc, const void *data, size_t len);
static inline uint32_t bl_crc32_final(uint32_t crc) { return crc ^ 0xFFFFFFFFU; }

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_CRC32_H
