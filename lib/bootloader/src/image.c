#include "bootloader/image.h"
#include "bootloader/crc32.h"

// validate a stored image: magic, header crc (over the header with header_crc32
// zeroed), and the app-image crc over `length` bytes at base+BL_IMAGE_OFFSET.
// all memory-mapped, so this is plain pointer access.
int bl_image_validate(const void *base) {
  const bl_image_header *h = (const bl_image_header *)base;
  if (h->magic != BL_IMAGE_MAGIC) {
    return -1;
  }

  bl_image_header tmp = *h;
  tmp.header_crc32 = 0U;
  if (bl_crc32(&tmp, sizeof(tmp)) != h->header_crc32) {
    return -2;
  }

  const uint8_t *img = (const uint8_t *)base + BL_IMAGE_OFFSET;
  if (bl_crc32(img, h->length) != h->image_crc32) {
    return -3;
  }
  return 0;
}
