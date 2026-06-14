/**
 * @file qspi_memmap.h
 * @brief memory-mapped QSPI flash storage driver
 *
 * brings up a QSPI flash (currently W25Q-family, via the w25qxx QSPI ops) on
 * the STM32 QUADSPI peripheral and exposes it as a memory-mapped window
 * (0x90000000 on the H7), so firmware can read it with plain pointer/memcpy
 * access.
 *
 * the read path is selected by a dynamic config (single / dual / quad lines)
 * so widening to 4-lane later is a config change, not a rewrite. dual is the
 * reliable default on the current board; quad needs better clock signal
 * integrity (see notes on the prescaler below).
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "hal.h"
#include "drivers/w25qxx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief dynamic configuration for a memory-mapped QSPI flash
 */
typedef struct {
  /** @brief ChibiOS WSPI driver, e.g. &WSPID1 */
  WSPIDriver *wspi;

  /** @brief WSPI peripheral config (DCR: FSIZE / CSHT) passed to wspiStart */
  const WSPIConfig *wspi_cfg;

  /** @brief memory-mapped window base address (0x90000000 on STM32H7) */
  uint32_t base;

  /** @brief device capacity in bytes (used for bounds checks) */
  uint32_t size_bytes;

  /**
   * @brief data-line width for fast reads (single / dual / quad)
   *
   * dual is reliable at the default clock on the reference board; quad needs
   * CLK signal integrity work first. QUAD automatically enables the QE bit
   * during init.
   */
  w25qxx_qspi_lines_t lines;

  /*
   * note: the QSPI clock is the QUADSPI kernel clock divided by
   * STM32_WSPI_QUADSPI1_PRESCALER_VALUE in mcuconf.h. the ChibiOS WSPI driver
   * fixes the prescaler at wspiStart, so the clock is a build-time setting, not
   * a runtime field here. /16 (~15 MHz) is the reliable ceiling on breadboard
   * wiring; faster needs better signal integrity.
   */
} qspi_memmap_config_t;

/**
 * @brief bring up the device: wspiStart, reset, verify a Winbond JEDEC id,
 *        and set the QE bit if the configured line mode needs it.
 * @note  does not enter memory-mapped mode.
 * @return true on success
 */
bool qspi_memmap_init(const qspi_memmap_config_t *cfg);

/**
 * @brief erase a single 4KB sector containing @p off (indirect mode)
 * @return true on success
 */
bool qspi_memmap_erase_sector(const qspi_memmap_config_t *cfg, uint32_t off);

/**
 * @brief program up to one 256-byte page at @p off (indirect mode). the target
 *        range must already be erased and must not cross a page boundary.
 * @return true on success
 */
bool qspi_memmap_program(const qspi_memmap_config_t *cfg, uint32_t off,
                         const uint8_t *buf, size_t len);

/**
 * @brief enter memory-mapped mode using the configured read line width.
 * @return the mapped base pointer (== cfg->base) or NULL on bad args.
 * @note   while mapped, no indirect command (init/erase/program) may run until
 *         qspi_memmap_disable() is called.
 */
const volatile void *qspi_memmap_enable(const qspi_memmap_config_t *cfg);

/**
 * @brief leave memory-mapped mode and return to indirect command mode
 */
void qspi_memmap_disable(const qspi_memmap_config_t *cfg);

/**
 * @brief invalidate the d-cache over a mapped region before reading it.
 *
 * the mapped window is cacheable and the cortex-m7 d-cache is enabled, so a
 * re-read of previously fetched addresses can return stale data without this.
 * @p addr and @p len should be 32-byte aligned to avoid touching neighbours.
 */
void qspi_memmap_invalidate(const void *addr, size_t len);

#ifdef __cplusplus
}
#endif
