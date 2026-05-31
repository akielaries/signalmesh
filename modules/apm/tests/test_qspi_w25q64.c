// memory-mapped QSPI proof on STM32H755 with a Winbond W25Q64
//
// goal: bring the QUADSPI peripheral up, talk to the W25Q64 via the
// ChibiOS WSPI driver, program a known pattern, then enter memory-mapped
// mode and verify the pattern by reading directly from 0x90000000
//
// what this file alone does NOT do (must be done outside this file):
//   1. modules/apm/bsp/halconf.h        -> HAL_USE_WSPI must be TRUE
//   2. modules/apm/bsp/mcuconf.h        -> STM32_WSPI_USE_QUADSPI1 must be TRUE
//   3. modules/apm/bsp/bsp.c or a new   -> configure the QUADSPI pins
//      bsp_wspi_config.c                   (CLK/NCS/IO0..IO3) as alt-func
//                                          for QUADSPI on this board's W25Q64
//                                          wiring, then wspiStart(&WSPID1, ...)
//   4. modules/apm/CMakeLists.txt       -> uncomment / add this file in SOURCES
//                                          and comment out any other test main
//
// reference paths inside the chibios tree (read for context):
//   os/hal/include/hal_wspi.h                       public WSPI api
//   os/hal/src/hal_wspi.c                           wspiMapFlash/wspiUnmapFlash
//   os/hal/ports/STM32/LLD/QUADSPIv2/hal_wspi_lld.* H7 quadspi low level
//
// hardware notes for the W25Q64 (read the datasheet for exact dummy counts):
//   - QE bit in Status Register 2 must be set before any 4-line command works
//   - 0x6B = Fast Read Quad Output, 1-1-4, 8 dummy cycles, simple to start with
//   - 0xEB = Fast Read Quad I/O,   1-4-4, mode byte + dummies, fastest

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/w25qxx.h"

#include "common/utils.h"

// memory-mapped window for QUADSPI on STM32H7
#define QSPI_MEMMAP_BASE 0x90000000UL

// where in flash we will erase + program our test pattern
#define TEST_SECTOR_OFFSET 0x00000000UL
#define TEST_BYTES         256U

// W25Q64 quad-enable lives in status register 2 bit 1
#define W25Q_SR2_QE_BIT 0x02U

// MDMA cannot access DTCM (where main()'s stack lives on H7), so all WSPI
// rx/tx buffers must be in AXI SRAM. file-scope statics land in .bss which
// the H7 linker script places in AXI SRAM (0x24000000), which MDMA can reach
static uint8_t s_jedec[3];
static uint8_t s_pattern[TEST_BYTES];
static uint8_t s_mapped_copy[TEST_BYTES];
static uint8_t s_sr_scratch[2];


// ---------------------------------------------------------------------------
// WSPIConfig: this should eventually live in bsp/configs/bsp_wspi_config.c
// kept here while scaffolding so the file is self-contained
// ---------------------------------------------------------------------------
static const WSPIConfig wspicfg = {
  .end_cb   = NULL,
  .error_cb = NULL,
  // DCR fields per RM0399 sec QUADSPI control register / DCR
  //   FSIZE = log2(device_size_in_bytes) - 1, for 8 MiB W25Q64 -> 22
  //   CSHT  = NCS high time between commands, conservative = 2 cycles -> 1
  .dcr      = STM32_DCR_FSIZE(22U) |
              STM32_DCR_CSHT(1U),
};

// helper: build a wspi_command_t for a 1-line opcode-only command (no addr, no data)
static void cmd_op_only(wspi_command_t *cmd, uint8_t op) {
  cmd->cmd   = op;
  cmd->addr  = 0U;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
             | WSPI_CFG_ADDR_MODE_NONE
             | WSPI_CFG_ALT_MODE_NONE
             | WSPI_CFG_DATA_MODE_NONE
             | WSPI_CFG_CMD_SIZE_8;
}

// helper: 1-line opcode + 1-line data read of N bytes (used for JEDEC id, status)
static void cmd_op_read1(wspi_command_t *cmd, uint8_t op) {
  cmd->cmd   = op;
  cmd->addr  = 0U;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
             | WSPI_CFG_ADDR_MODE_NONE
             | WSPI_CFG_ALT_MODE_NONE
             | WSPI_CFG_DATA_MODE_ONE_LINE
             | WSPI_CFG_CMD_SIZE_8;
}

// helper: 1-line opcode + 1-line 24-bit address + 1-line data write (page program 0x02)
static void cmd_op_addr_write1(wspi_command_t *cmd, uint8_t op, uint32_t addr) {
  cmd->cmd   = op;
  cmd->addr  = addr;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
             | WSPI_CFG_ADDR_MODE_ONE_LINE
             | WSPI_CFG_ADDR_SIZE_24
             | WSPI_CFG_ALT_MODE_NONE
             | WSPI_CFG_DATA_MODE_ONE_LINE
             | WSPI_CFG_CMD_SIZE_8;
}

// helper: 1-line opcode + 1-line address only (no data, e.g. sector erase 0x20)
static void cmd_op_addr_only(wspi_command_t *cmd, uint8_t op, uint32_t addr) {
  cmd->cmd   = op;
  cmd->addr  = addr;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
             | WSPI_CFG_ADDR_MODE_ONE_LINE
             | WSPI_CFG_ADDR_SIZE_24
             | WSPI_CFG_ALT_MODE_NONE
             | WSPI_CFG_DATA_MODE_NONE
             | WSPI_CFG_CMD_SIZE_8;
}

// helper: the read command used in memory-mapped mode
// 0x6B Fast Read Quad Output: cmd 1-line, addr 1-line, data 4-line, 8 dummies
static void cmd_mmap_read_1_1_4(wspi_command_t *cmd) {
  cmd->cmd   = W25QXX_CMD_FAST_READ_QUAD;
  cmd->addr  = 0U;
  cmd->alt   = 0U;
  cmd->dummy = 8U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
             | WSPI_CFG_ADDR_MODE_ONE_LINE
             | WSPI_CFG_ADDR_SIZE_24
             | WSPI_CFG_ALT_MODE_NONE
             | WSPI_CFG_DATA_MODE_FOUR_LINES
             | WSPI_CFG_CMD_SIZE_8;
}

// ---------------------------------------------------------------------------
// flash helpers
// ---------------------------------------------------------------------------

static bool flash_wait_idle(uint32_t timeout_ms) {
  wspi_command_t cmd;
  systime_t deadline = chVTGetSystemTimeX() + TIME_MS2I(timeout_ms);

  cmd_op_read1(&cmd, W25QXX_CMD_READ_STATUS_REG1);
  do {
    if (wspiReceive(&WSPID1, &cmd, 1U, s_sr_scratch) != MSG_OK) {
      return false;
    }
    if ((s_sr_scratch[0] & W25QXX_STATUS_BUSY) == 0U) {
      return true;
    }
    chThdSleepMilliseconds(1);
  } while (chVTGetSystemTimeX() < deadline);
  return false;
}

static bool flash_write_enable(void) {
  wspi_command_t cmd;
  cmd_op_only(&cmd, W25QXX_CMD_WRITE_ENABLE);
  return wspiCommand(&WSPID1, &cmd) == MSG_OK;
}

// reads the 3-byte JEDEC id (mfg, type, capacity)
static bool flash_read_jedec_id(uint8_t out[3]) {
  wspi_command_t cmd;
  cmd_op_read1(&cmd, W25QXX_CMD_READ_JEDEC_ID);
  return wspiReceive(&WSPID1, &cmd, 3U, out) == MSG_OK;
}

// sets the QE bit in SR2 if not already set
// uses 0x31 write-status-register-2 (Winbond extension to W25QXX_CMD_WRITE_STATUS_REG)
static bool flash_enable_quad(void) {
  wspi_command_t cmd;

  cmd_op_read1(&cmd, W25QXX_CMD_READ_STATUS_REG2);
  if (wspiReceive(&WSPID1, &cmd, 1U, s_sr_scratch) != MSG_OK) {
    return false;
  }
  if (s_sr_scratch[0] & W25Q_SR2_QE_BIT) {
    return true;
  }

  if (!flash_write_enable()) {
    return false;
  }

  s_sr_scratch[0] |= W25Q_SR2_QE_BIT;
  cmd.cmd   = 0x31U;   // write SR2 (Winbond)
  cmd.addr  = 0U;
  cmd.alt   = 0U;
  cmd.dummy = 0U;
  cmd.cfg   = WSPI_CFG_CMD_MODE_ONE_LINE
            | WSPI_CFG_ADDR_MODE_NONE
            | WSPI_CFG_ALT_MODE_NONE
            | WSPI_CFG_DATA_MODE_ONE_LINE
            | WSPI_CFG_CMD_SIZE_8;
  if (wspiSend(&WSPID1, &cmd, 1U, s_sr_scratch) != MSG_OK) {
    return false;
  }
  return flash_wait_idle(1000);
}

static bool flash_sector_erase(uint32_t addr) {
  wspi_command_t cmd;
  if (!flash_write_enable()) {
    return false;
  }
  cmd_op_addr_only(&cmd, W25QXX_CMD_SECTOR_ERASE, addr);
  if (wspiCommand(&WSPID1, &cmd) != MSG_OK) {
    return false;
  }
  return flash_wait_idle(1000);
}

static bool flash_page_program(uint32_t addr, const uint8_t *buf, size_t len) {
  wspi_command_t cmd;
  if (!flash_write_enable()) {
    return false;
  }
  cmd_op_addr_write1(&cmd, W25QXX_CMD_PAGE_PROGRAM, addr);
  if (wspiSend(&WSPID1, &cmd, len, buf) != MSG_OK) {
    return false;
  }
  return flash_wait_idle(100);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_qspi_w25q64: memory-mapped QSPI proof ---\r\n");

  // TODO move into bsp_init once stable
  bsp_printf("> before wspiStart\n");
  wspiStart(&WSPID1, &wspicfg);
  bsp_printf("> after  wspiStart, state=%d\n", (int)WSPID1.state);

  // phase 1: prove the indirect path works
  bsp_printf("> before flash_read_jedec_id\n");
  if (!flash_read_jedec_id(s_jedec)) {
    bsp_printf("FAIL: could not read JEDEC id (indirect path broken)\n");
    return -1;
  }
  bsp_printf("> after  flash_read_jedec_id\n");
  bsp_printf("JEDEC id: %02X %02X %02X (expect EF 40 17 for W25Q64FV)\n",
             s_jedec[0], s_jedec[1], s_jedec[2]);
  if (s_jedec[0] != W25QXX_MFG_WINBOND_ID) {
    bsp_printf("FAIL: not a winbond part - check wiring and dummy bytes\n");
    return -1;
  }

  // phase 2: enable quad mode (required for 0x6B and 0xEB read paths)
  if (!flash_enable_quad()) {
    bsp_printf("FAIL: could not set QE bit in SR2\n");
    return -1;
  }
  bsp_printf("QE bit set\n");

  // phase 3: write a known pattern via indirect
  for (size_t i = 0; i < TEST_BYTES; i++) {
    s_pattern[i] = (uint8_t)(0xA0 ^ (i & 0xFF));
  }
  if (!flash_sector_erase(TEST_SECTOR_OFFSET)) {
    bsp_printf("FAIL: sector erase\n");
    return -1;
  }
  if (!flash_page_program(TEST_SECTOR_OFFSET, s_pattern, TEST_BYTES)) {
    bsp_printf("FAIL: page program\n");
    return -1;
  }
  bsp_printf("programmed %u bytes at 0x%08lX\n",
             (unsigned)TEST_BYTES, (unsigned long)TEST_SECTOR_OFFSET);

  // phase 4: enter memory-mapped mode and verify via direct pointer read
  // note on cache:
  //   the cortex-m7 d-cache may shadow stale values for the 0x90000000 window
  //   if not invalidated. for a first-pass proof we either disable d-cache, or
  //   call SCB_InvalidateDCache_by_Addr after wspiMapFlash. simplest scaffold
  //   below assumes d-cache is off (chibios default unless you opt in)
  //
  // also: while mapped, you CANNOT issue indirect commands on WSPID1 without
  // calling wspiUnmapFlash first
  wspi_command_t map_cmd;
  cmd_mmap_read_1_1_4(&map_cmd);
  wspiMapFlash(&WSPID1, &map_cmd, NULL);

  const volatile uint8_t *mapped = (const volatile uint8_t *)(QSPI_MEMMAP_BASE + TEST_SECTOR_OFFSET);
  for (size_t i = 0; i < TEST_BYTES; i++) {
    s_mapped_copy[i] = mapped[i];
  }

  wspiUnmapFlash(&WSPID1);

  // verify
  if (memcmp(s_pattern, s_mapped_copy, TEST_BYTES) != 0) {
    bsp_printf("FAIL: memory-mapped read does not match indirect write\n");
    print_hexdump("written ", s_pattern,     32);
    print_hexdump("mapped  ", s_mapped_copy, 32);
    return -1;
  }

  bsp_printf("PASS: %u bytes at 0x%08lX match between indirect-write and mmap-read\n",
             (unsigned)TEST_BYTES,
             (unsigned long)(QSPI_MEMMAP_BASE + TEST_SECTOR_OFFSET));
  print_hexdump("mapped (head)", s_mapped_copy, 32);

  bsp_printf("\n--- test_qspi_w25q64 finished ---\r\n");
  while (1) {
    chThdSleepMilliseconds(1000);
  }
}
