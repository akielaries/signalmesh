// signalmesh system memory map (single map - the STM32/APM's view). the APM is
// the STM32H755 manager; the ACM is the FPGA it drives. the FPGA bitstream is
// just an asset the STM32 stores in QSPI.
//
// internal flash (2MB, dual bank): bank 1 (0x08000000) holds the bootloader;
//   bank 2 (0x08100000) is the app exec region. the app is linked once for
//   0x08100000; whichever QSPI app slot the bootloader picks is copied here to
//   run. exec lives in bank 2 on purpose: the bootloader runs from bank 1 and
//   the H7 supports read-while-write ACROSS banks, so it can erase/program the
//   exec region without stalling its own fetch (same-bank RWW would fault).
// QSPI (16MB dual-SPI, memory-mapped at 0x90000000, verified by test_qspi_bench):
//   holds the two app images (slot0 fallback / slot1 active), the golden
//   bootloader, the fpga bitstreams, and params. fpga slots sized for the
//   largest bitstream we target (GW5A-25 .bin ~742KB).

#include "bootloader/memmap.h"

const bl_region bl_memmap[BL_SLOT_COUNT] = {
  [BL_SLOT_APP_EXEC]    = {0x08100000U, 0x00100000U, "app_exec (iflash bank2)"}, // 1M
  [BL_SLOT_APP_0]       = {0x90000000U, 0x00100000U, "app_0 (fallback)"},  // 1M
  [BL_SLOT_APP_1]       = {0x90100000U, 0x00100000U, "app_1 (active)"},    // 1M
  [BL_SLOT_BL_GOLDEN]   = {0x90200000U, 0x00040000U, "bl_golden"},         // 256K
  [BL_SLOT_FPGA_ACTIVE] = {0x90240000U, 0x00100000U, "fpga_active"},       // 1M
  [BL_SLOT_FPGA_GOLDEN] = {0x90340000U, 0x00100000U, "fpga_golden"},       // 1M
  [BL_SLOT_PARAMS]      = {0x90440000U, 0x00010000U, "params"},            // 64K
};
