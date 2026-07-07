// minimal STM32H7 internal-flash erase/program (dual-bank, 128KB sectors,
// 256-bit flash words). used by the bootloader to stage a validated app image
// out of QSPI into the internal-flash exec region.
//
// RWW note: the H7 supports read-while-write ACROSS banks - the bootloader runs
// from bank 1 (0x08000000) and can erase/program bank 2 (0x08100000) without
// stalling its own instruction fetch. programming a sector in the SAME bank the
// code executes from would fault, so keep the exec region in the other bank.

#ifndef DRIVERS_STM32H7_FLASH_H
#define DRIVERS_STM32H7_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STM32H7_FLASH_SECTOR_SIZE 0x20000U // 128KB per sector
#define STM32H7_FLASH_WORD_BYTES  32U      // 256-bit flash word (program granule)

// erase every 128KB sector overlapping [addr, addr+len). addr/len need not be
// sector-aligned; whole sectors are erased. the range must lie within one bank.
// returns 0 on success, negative on error.
int stm32h7_flash_erase(uint32_t addr, uint32_t len);

// program len bytes from src to internal-flash addr. addr must be 32-byte
// aligned (flash-word boundary); a short tail is padded with 0xFF. the target
// must be pre-erased. returns 0 on success, negative on error.
int stm32h7_flash_program(uint32_t addr, const void *src, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // DRIVERS_STM32H7_FLASH_H
