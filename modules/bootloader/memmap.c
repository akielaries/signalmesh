// signalmesh system memory map (single map - the STM32/APM's view). the APM is
// the STM32H755 manager; the ACM is the FPGA it drives. the FPGA bitstream is
// just an asset the STM32 stores in QSPI.
//
// internal flash (2MB, dual bank): [bootloader 128K @ 0x08000000][app exec
//   region @ 0x08020000]. the app is linked once for 0x08020000; whichever QSPI
//   app slot the bootloader picks is copied here to run.
//   TODO: the app linker script must place .text at 0x08020000.
// QSPI (16MB dual-SPI, memory-mapped at 0x90000000, verified by test_qspi_bench):
//   holds the two app images (slot0 fallback / slot1 active), the golden
//   bootloader, the fpga bitstreams, and params. fpga slots sized for the
//   largest bitstream we target (GW5A-25 .bin ~742KB).

#include "bootloader/memmap.h"

const bl_region bl_memmap[BL_SLOT_COUNT] = {
  [BL_SLOT_APP_EXEC]    = {0x08020000U, 0x000E0000U, "app_exec (iflash)"}, // ~896K
  [BL_SLOT_APP_0]       = {0x90000000U, 0x00100000U, "app_0 (fallback)"},  // 1M
  [BL_SLOT_APP_1]       = {0x90100000U, 0x00100000U, "app_1 (active)"},    // 1M
  [BL_SLOT_BL_GOLDEN]   = {0x90200000U, 0x00040000U, "bl_golden"},         // 256K
  [BL_SLOT_FPGA_ACTIVE] = {0x90240000U, 0x00100000U, "fpga_active"},       // 1M
  [BL_SLOT_FPGA_GOLDEN] = {0x90340000U, 0x00100000U, "fpga_golden"},       // 1M
  [BL_SLOT_PARAMS]      = {0x90440000U, 0x00010000U, "params"},            // 64K
};
