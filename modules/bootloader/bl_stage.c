// see bl_stage.h. copies a slot's app image [BL_IMAGE_OFFSET .. +length) from
// QSPI into the internal-flash exec region and verifies it. the stored image is
// the raw app binary starting with the app's vector table, so it lands at the
// exec base (the app is linked once for that address) - the slot's image header
// stays behind in QSPI as metadata.

#include "hal.h" // SCB_CleanInvalidateDCache

#include "bl_stage.h"

#include "bootloader/image.h"
#include "bootloader/crc32.h"
#include "drivers/stm32h7_flash.h"

#include "bsp/utils/bsp_io.h"

int bl_stage_app(enum bl_slot slot) {
  const uint8_t *slot_base = (const uint8_t *)(uintptr_t)bl_memmap[slot].base;
  const bl_image_header *h = (const bl_image_header *)slot_base;
  const uint8_t *img = slot_base + BL_IMAGE_OFFSET;

  uint32_t exec = bl_memmap[BL_SLOT_APP_EXEC].base;
  uint32_t exec_size = bl_memmap[BL_SLOT_APP_EXEC].size;

  if (h->length > exec_size) {
    bsp_printf("stage: image %lu > exec region %lu\r\n",
               (unsigned long)h->length, (unsigned long)exec_size);
    return -1;
  }

  bsp_printf("stage: %s -> exec 0x%08lX (%lu bytes)\r\n", bl_memmap[slot].name,
             (unsigned long)exec, (unsigned long)h->length);

  if (stm32h7_flash_erase(exec, h->length) != 0) {
    bsp_printf("stage: erase failed\r\n");
    return -2;
  }
  if (stm32h7_flash_program(exec, img, h->length) != 0) {
    bsp_printf("stage: program failed\r\n");
    return -3;
  }

  // the program path writes via the flash controller, bypassing the D-cache;
  // drop stale lines so the verify (and the app's execution) see real flash
  SCB_CleanInvalidateDCache();
  SCB_InvalidateICache();

  if (bl_crc32((const void *)(uintptr_t)exec, h->length) != h->image_crc32) {
    bsp_printf("stage: verify crc mismatch\r\n");
    return -4;
  }
  return 0;
}
