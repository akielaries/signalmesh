#include "drivers/qspi_memmap.h"

#include "ch.h"
#include "hal.h"

#include "drivers/w25qxx.h"
#include "bsp/utils/bsp_io.h"

// memory-mapped QSPI flash storage driver. the flash command primitives live
// in the w25qxx driver (w25qxx_qspi_*); this layer owns bring-up, the dynamic
// read-mode selection, and the map/unmap + cache handling

bool qspi_memmap_init(const qspi_memmap_config_t *cfg) {
  uint8_t jedec[3];

  if (cfg == NULL || cfg->wspi == NULL || cfg->wspi_cfg == NULL) {
    return false;
  }

  wspiStart(cfg->wspi, cfg->wspi_cfg);

  // reset in case a prior attempt left the chip mid-command
  if (!w25qxx_qspi_reset(cfg->wspi)) {
    return false;
  }

  if (!w25qxx_qspi_read_jedec(cfg->wspi, jedec)) {
    return false;
  }
  if (jedec[0] != W25QXX_MFG_WINBOND_ID) {
    bsp_printf("qspi_memmap: unexpected JEDEC %02X %02X %02X\n",
               jedec[0], jedec[1], jedec[2]);
    return false;
  }

  // quad reads need the QE bit; single/dual do not
  if (w25qxx_qspi_lines_need_qe(cfg->lines)) {
    if (!w25qxx_qspi_set_quad_enable(cfg->wspi)) {
      return false;
    }
  }

  return true;
}

bool qspi_memmap_erase_sector(const qspi_memmap_config_t *cfg, uint32_t off) {
  if (cfg == NULL || off >= cfg->size_bytes) {
    return false;
  }
  return w25qxx_qspi_erase_sector(cfg->wspi, off);
}

bool qspi_memmap_program(const qspi_memmap_config_t *cfg, uint32_t off,
                         const uint8_t *buf, size_t len) {
  if (cfg == NULL || buf == NULL || len == 0U) {
    return false;
  }
  if (off + len > cfg->size_bytes) {
    return false;
  }
  return w25qxx_qspi_program_page(cfg->wspi, off, buf, len);
}

const volatile void *qspi_memmap_enable(const qspi_memmap_config_t *cfg) {
  wspi_command_t cmd;

  if (cfg == NULL || cfg->wspi == NULL) {
    return NULL;
  }

  // address is supplied per-access by the memory-mapped engine, so build the
  // read command with address 0
  w25qxx_qspi_build_read_cmd(&cmd, cfg->lines, 0U);
  wspiMapFlash(cfg->wspi, &cmd, NULL);

  return (const volatile void *)cfg->base;
}

void qspi_memmap_disable(const qspi_memmap_config_t *cfg) {
  if (cfg == NULL || cfg->wspi == NULL) {
    return;
  }
  wspiUnmapFlash(cfg->wspi);
}

void qspi_memmap_invalidate(const void *addr, size_t len) {
  cacheBufferInvalidate(addr, len);
}
