// storage-map structure. the SET of slots is generic; the actual base addresses
// and sizes are the single system map, defined in modules/bootloader/memmap.c
// (the STM32/APM's view - the FPGA/ACM bitstream is an asset it stores in QSPI).
//
// app model: two app IMAGES live in QSPI (slot 0 = last-known-good fallback,
// slot 1 = newest/active, the update target). the bootloader copies the chosen
// valid slot into a single fixed internal-flash EXEC region and jumps - so the
// app is linked once (for the exec address) and the qspi->qspi slot rotation is
// a plain copy, no relink. the bootloader itself at 0x08000000 is the golden
// image (recovery); if no app slot validates, it stays resident.

#ifndef BOOTLOADER_MEMMAP_H
#define BOOTLOADER_MEMMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum bl_slot {
  BL_SLOT_APP_EXEC = 0, // internal flash: where the chosen app runs, linked once
  BL_SLOT_APP_0,        // qspi: app image fallback (last-known-good)
  BL_SLOT_APP_1,        // qspi: app image active (newest; update target)
  BL_SLOT_BL_GOLDEN,    // qspi: recovery copy of the bootloader
  BL_SLOT_FPGA_ACTIVE,  // qspi: current fpga bitstream (.bin)
  BL_SLOT_FPGA_GOLDEN,  // qspi: recovery fpga bitstream
  BL_SLOT_PARAMS,       // qspi: active-slot pointer + boot-confirmed flags
  BL_SLOT_COUNT,
};

typedef struct {
  uint32_t base;     // absolute address (internal flash or memory-mapped qspi)
  uint32_t size;     // bytes reserved for this slot
  const char *name;
} bl_region;

// defined per system; index with enum bl_slot
extern const bl_region bl_memmap[BL_SLOT_COUNT];

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_MEMMAP_H
