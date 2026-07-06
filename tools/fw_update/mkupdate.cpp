// mkupdate: wrap a raw app .bin into a bootloader image blob the bootloader can
// validate and XIP-boot. output layout matches the QSPI slot:
//   [bl_image_header][pad to BL_IMAGE_OFFSET][app image]
// the header carries the app length + crc32 so the bootloader can verify the
// stored image independently of the transfer. update.bin --file sends this blob.
//
// usage: ./mkupdate.bin <app.bin> <out.smup> [APM|GW2AR18|GW5A25]

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "bootloader/protocol.h" // enum bl_target
#include "bootloader/image.h"    // bl_image_header, BL_IMAGE_MAGIC, BL_IMAGE_OFFSET
#include "bootloader/crc32.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <app.bin> <out.smup> [APM|GW2AR18|GW5A25]\n",
            argv[0]);
    return 2;
  }

  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    fprintf(stderr, "open %s failed\n", argv[1]);
    return 1;
  }
  std::vector<uint8_t> app((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

  uint16_t target = BL_TARGET_APM_H755;
  if (argc >= 4) {
    std::string t = argv[3];
    if (t == "GW2AR18") {
      target = BL_TARGET_FPGA_GW2AR18;
    } else if (t == "GW5A25") {
      target = BL_TARGET_FPGA_GW5A25;
    }
  }

  bl_image_header h{};
  h.magic = BL_IMAGE_MAGIC;
  h.target = target;
  h.flags = 0;
  h.version = 1;
  h.length = static_cast<uint32_t>(app.size());
  h.image_crc32 = bl_crc32(app.data(), app.size());
  h.header_crc32 = 0;
  h.header_crc32 = bl_crc32(&h, sizeof(h)); // over the header with the field zeroed

  // blob: header at 0, app at BL_IMAGE_OFFSET, gap zero-filled
  std::vector<uint8_t> blob(BL_IMAGE_OFFSET + app.size(), 0);
  memcpy(blob.data(), &h, sizeof(h));
  memcpy(blob.data() + BL_IMAGE_OFFSET, app.data(), app.size());

  std::ofstream out(argv[2], std::ios::binary);
  out.write(reinterpret_cast<const char *>(blob.data()),
            static_cast<std::streamsize>(blob.size()));
  if (!out) {
    fprintf(stderr, "write %s failed\n", argv[2]);
    return 1;
  }

  printf("wrote %s: image %zu bytes @ +0x%X, blob %zu bytes, target %u, "
         "image crc 0x%08X\n",
         argv[2], app.size(), BL_IMAGE_OFFSET, blob.size(), target,
         h.image_crc32);
  return 0;
}
