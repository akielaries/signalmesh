// stage a validated app image out of a QSPI slot into the internal-flash exec
// region, so the app runs at full core speed from flash instead of XIP from the
// slow QSPI. the QSPI must be memory-mapped (the source is read by pointer).

#ifndef BL_STAGE_H
#define BL_STAGE_H

#include "bootloader/memmap.h"

// copy the app image from `slot` (a QSPI app slot) into BL_SLOT_APP_EXEC:
// erase the exec sectors, program the image, then crc-verify the flash copy.
// the image must already have passed bl_image_validate. returns 0 on success,
// negative on error (image too big / erase / program / verify).
int bl_stage_app(enum bl_slot slot);

#endif // BL_STAGE_H
