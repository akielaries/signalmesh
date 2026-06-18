#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/w25qxx.h"
#include "ch.h"
#include "hal.h"
#include "drivers/spi.h" // include for spi_bus_acquire/release etc.


// forward declarations for driver_t interface
static int w25qxx_init(device_t *dev);
static int w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count);
static int w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count);

// forward declarations for internal helper functions and erase operations
static int w25qxx_wait_busy(w25qxx_t *flash_dev);
static int w25qxx_write_enable(w25qxx_t *flash_dev);
static int w25qxx_write_disable(w25qxx_t *flash_dev);
static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num);
static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id);

// erase functions (exposed in w25qxx.h)
int w25qxx_sector_erase(w25qxx_t *flash_dev, uint32_t sector_addr);
int w25qxx_block_erase(w25qxx_t *flash_dev, uint32_t block_addr, uint32_t block_size);

// driver descriptor
const driver_t w25qxx_driver __attribute__((used)) = {
  .name               = "w25qxx",
  .init               = w25qxx_init,
  .read               = w25qxx_read,
  .write              = w25qxx_write,
  .poll               = NULL,
  .readings_directory = NULL,
};


static int w25qxx_wait_busy(w25qxx_t *flash_dev) {
  // microseconds?
  uint32_t timeout = 10000;
  CC_ALIGN_DATA(32) static uint8_t status[2];
  CC_ALIGN_DATA(32) static uint8_t cmd[2];

  while (timeout != 0) {
    cmd[0] = W25QXX_CMD_READ_STATUS_REG1;

    spi_bus_exchange(&flash_dev->bus, cmd, status, 2);

    if ((status[1] & 0x01) == 0x00) {
      break;
    }
    timeout--;
    chThdSleepMicroseconds(10);
  }

  return DRIVER_OK;
}

static int w25qxx_write_enable(w25qxx_t *flash_dev) {
  CC_ALIGN_DATA(32) static uint8_t cmd[1];
  CC_ALIGN_DATA(32) static uint8_t reply[1];
  cmd[0] = W25QXX_CMD_WRITE_ENABLE;

  spi_bus_exchange(&flash_dev->bus, cmd, reply, 1);

  chThdSleepMicroseconds(50);

  // verify write enable bit is set
  if ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_WEL) == 0) {
    bsp_printf("W25QXX: Failed to enable write (WEL not set).\n");
    return DRIVER_ERROR;
  }

  return DRIVER_OK;
}

static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num) {
  CC_ALIGN_DATA(32) static uint8_t cmd[2]; // Need 2 bytes for the exchange
  CC_ALIGN_DATA(32) static uint8_t rx_buf[2];

  switch (reg_num) {
    case 1:
      cmd[0] = W25QXX_CMD_READ_STATUS_REG1;
      break;
    case 2:
      cmd[0] = W25QXX_CMD_READ_STATUS_REG2;
      break;
    case 3:
      cmd[0] = W25QXX_CMD_READ_STATUS_REG3;
      break;
    default:
      return 0xFF; // error indication
  }

  // A 2-byte exchange is required.
  // Byte 1: Send command, receive garbage.
  // Byte 2: Send dummy, receive status.
  spi_bus_exchange(&flash_dev->bus, cmd, rx_buf, 2);

  // The actual status is in the second byte received.
  return rx_buf[1];
}

static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id) {
  CC_ALIGN_DATA(32) static uint8_t cmd[1];
  CC_ALIGN_DATA(32) static uint8_t reply[4];
  cmd[0] = W25QXX_CMD_READ_JEDEC_ID;

  spi_bus_exchange(&flash_dev->bus, cmd, reply, 4);

  // the first received byte is garbage, ID is in bytes 2, 3, 4
  id->manufacturer_id = reply[1];
  id->device_id_high  = reply[2];
  id->device_id_low   = reply[3];

  return DRIVER_OK;
}

static int w25qxx_erase_sector(w25qxx_t *flash, uint32_t addr) {
  CC_ALIGN_DATA(32) static uint8_t cmd[4];
  CC_ALIGN_DATA(32) static uint8_t reply[4];

  if (w25qxx_write_enable(flash) != DRIVER_OK) {
    return DRIVER_ERROR;
  }

  addr &= ~(4096 - 1); // align to 4KB

  cmd[0] = W25QXX_CMD_SECTOR_ERASE;
  cmd[1] = (addr >> 16) & 0xFF;
  cmd[2] = (addr >> 8) & 0xFF;
  cmd[3] = (addr >> 0) & 0xFF;

  spi_bus_exchange(&flash->bus, cmd, reply, 4);

  return w25qxx_wait_busy(flash);
}

static void w25qxx_reset(w25qxx_t *flash_dev) {
  bsp_printf("performing reset\n");
  CC_ALIGN_DATA(32) static uint8_t cmd[4];
  CC_ALIGN_DATA(32) static uint8_t reply[4];

  cmd[0] = W25QXX_CMD_RELEASE_DEEP_POWER_DOWN;
  cmd[1] = 0xFF;
  cmd[2] = 0xFF;
  cmd[3] = 0xFF;
  spi_bus_exchange(&flash_dev->bus, cmd, reply, 4);
  bsp_printf("releasing from power down\n");

  // perform a software reset
  cmd[0] = W25QXX_CMD_RESET_ENABLE;
  spi_bus_exchange(&flash_dev->bus, cmd, reply, 1);

  cmd[0] = W25QXX_CMD_RESET_DEVICE;
  spi_bus_exchange(&flash_dev->bus, cmd, reply, 1);

  chThdSleepMilliseconds(10); // wait tRST
}

static int w25qxx_init(device_t *dev) {
  if (dev->priv == NULL) {
    return DRIVER_ERROR;
  }
  bsp_printf("W25QXX: Initializing...\n");
  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;

  // assign the SPI bus configuration
  flash_dev->bus = *(spi_bus_t *)dev->bus;

  w25qxx_reset(flash_dev);


  // read JEDEC ID to identify the device
  w25qxx_jedec_id_t jedec_id;
  if (w25qxx_read_jedec_id(flash_dev, &jedec_id) != DRIVER_OK) {
    return DRIVER_ERROR;
  }

  flash_dev->manufacturer_id = jedec_id.manufacturer_id;
  flash_dev->device_id       = (jedec_id.device_id_high << 8) | jedec_id.device_id_low;

  bsp_printf("mfg id: 0x%X\n", flash_dev->manufacturer_id);
  bsp_printf("dev id: 0x%X\n", flash_dev->device_id);

  switch (flash_dev->manufacturer_id) {
    case W25QXX_MFG_WINBOND_ID:
      bsp_printf("Windbond Serial Flash\n");
      break;
    default:
      bsp_printf("unknown W25XX IC\n");
      break;
  }

  // determine device size based on JEDEC ID
  switch (flash_dev->device_id) {
    case 0x4015: // W25Q16
      flash_dev->size_bytes = W25Q16_SIZE_BYTES;
      break;
    case 0x4016: // W25Q32
      flash_dev->size_bytes = W25Q32_SIZE_BYTES;
      break;
    case 0x4017: // W25Q64
      flash_dev->size_bytes = W25Q64_SIZE_BYTES;
      bsp_printf("FLash IC ID 0x%X w25q64. %d bytes / %d mb\n",
                 flash_dev->device_id,
                 W25Q64_SIZE_BYTES,
                 W25Q64_SIZE_BYTES / 1024 / 1024);
      break;
    case 0x4018: // W25Q128
      flash_dev->size_bytes = W25Q128_SIZE_BYTES;
      break;
    default:
      flash_dev->size_bytes = W25Q64_SIZE_BYTES; // default to 8MB if unknown
      break;
  }


  bsp_printf("W25QXX: Initialization complete.\n");
  return DRIVER_OK;
}

static int w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count) {
  if (dev == NULL || buf == NULL) {
    return DRIVER_INVALID_PARAM;
  }

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes) {
    return DRIVER_INVALID_PARAM;
  }


#define READ_CHUNK_SIZE W25QXX_PAGE_SIZE_BYTES
  CC_ALIGN_DATA(32) static uint8_t tx_buffer[4 + READ_CHUNK_SIZE];
  CC_ALIGN_DATA(32) static uint8_t rx_buffer[4 + READ_CHUNK_SIZE];

  size_t bytes_read = 0;
  uint8_t *data_ptr = (uint8_t *)buf;

  while (bytes_read < count) {
    size_t bytes_remaining = count - bytes_read;
    size_t chunk_size     = (bytes_remaining < READ_CHUNK_SIZE) ? bytes_remaining : READ_CHUNK_SIZE;
    uint32_t current_addr = offset + bytes_read;


    tx_buffer[0] = W25QXX_CMD_READ_DATA;
    // tx_buffer[1] = (uint8_t)(current_addr >> 24) & 0xFF;
    tx_buffer[1] = (current_addr >> 16) & 0xFF;
    tx_buffer[2] = (current_addr >> 8) & 0xFF;
    tx_buffer[3] = current_addr & 0xFF;

    // tx_buffer[4] = 0x00; // Dummy byte for Fast Read.

    spi_bus_exchange(&flash_dev->bus, tx_buffer, rx_buffer, 4 + chunk_size);

    memcpy(data_ptr + bytes_read, rx_buffer + 4, chunk_size);


    bytes_read += chunk_size;
  }

  return bytes_read;
}

static int w25qxx_page_program(w25qxx_t *flash, uint32_t addr, const void *buf, size_t size) {
    if (size > W25QXX_PAGE_SIZE_BYTES) {
        return DRIVER_INVALID_PARAM;
    }

    if (w25qxx_write_enable(flash) != DRIVER_OK) {
        return DRIVER_ERROR;
    }

    CC_ALIGN_DATA(32) static uint8_t tx_buffer[4 + W25QXX_PAGE_SIZE_BYTES];
    CC_ALIGN_DATA(32) static uint8_t rx_buffer[4 + W25QXX_PAGE_SIZE_BYTES];

    tx_buffer[0] = W25QXX_CMD_PAGE_PROGRAM;
    tx_buffer[1] = (addr >> 16) & 0xFF;
    tx_buffer[2] = (addr >> 8) & 0xFF;
    tx_buffer[3] = (addr >> 0) & 0xFF;

    memcpy(&tx_buffer[4], buf, size);

    spi_bus_exchange(&flash->bus, tx_buffer, rx_buffer, 4 + size);

    return w25qxx_wait_busy(flash);
}

static int w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count) {
    if (dev == NULL || buf == NULL) {
        return DRIVER_INVALID_PARAM;
    }

    w25qxx_t *flash = (w25qxx_t *)dev->priv;

    if (offset + count > flash->size_bytes) {
        return DRIVER_INVALID_PARAM;
    }

    // Buffer for sector-based read-modify-write
    CC_ALIGN_DATA(32) static uint8_t sector_buf[W25QXX_SECTOR_SIZE_BYTES];

    const uint8_t *data_ptr = (const uint8_t *)buf;
    size_t bytes_written = 0;

    while (bytes_written < count) {
        uint32_t current_addr = offset + bytes_written;
        uint32_t sector_addr = current_addr & ~(W25QXX_SECTOR_SIZE_BYTES - 1);
        uint32_t sector_internal_offset = current_addr - sector_addr;

        // Determine how many bytes to write in this sector
        size_t bytes_to_write_in_sector = W25QXX_SECTOR_SIZE_BYTES - sector_internal_offset;
        size_t remaining_bytes = count - bytes_written;
        if (bytes_to_write_in_sector > remaining_bytes) {
            bytes_to_write_in_sector = remaining_bytes;
        }

        // Read-Modify-Write: only if the write does not span the entire sector
        // from the beginning. A full sector erase is faster if we overwrite it all.
        // For simplicity and correctness under all chunking scenarios, we will always R-M-W.
        
        // 1. Read the entire sector into the buffer
        if (w25qxx_read(dev, sector_addr, sector_buf, W25QXX_SECTOR_SIZE_BYTES) != W25QXX_SECTOR_SIZE_BYTES) {
            return DRIVER_ERROR; // Failed to read the sector
        }

        // 2. Modify the buffer with the new data
        memcpy(&sector_buf[sector_internal_offset], data_ptr + bytes_written, bytes_to_write_in_sector);

        // 3. Erase the physical sector
        if (w25qxx_erase_sector(flash, sector_addr) != DRIVER_OK) {
            return DRIVER_ERROR; // Failed to erase
        }

        // 4. Write the modified sector back, page by page
        for (size_t page_offset = 0; page_offset < W25QXX_SECTOR_SIZE_BYTES; page_offset += W25QXX_PAGE_SIZE_BYTES) {
            if (w25qxx_page_program(flash, sector_addr + page_offset, &sector_buf[page_offset], W25QXX_PAGE_SIZE_BYTES) != DRIVER_OK) {
                return DRIVER_ERROR; // Failed to write page
            }
        }
        
        bytes_written += bytes_to_write_in_sector;
    }

    return bytes_written;
}

// ===========================================================================
// QSPI (ChibiOS WSPI) command-level operations. see w25qxx.h for cache and
// reentrancy notes. these back the memory-mapped storage driver (qspi_memmap)
// ===========================================================================

#define W25QXX_QSPI_READ_DUMMY 8U
#define W25Q_SR2_QE_BIT        0x02U

// shared internal aligned buffers (cortex-m7 d-cache line is 32 bytes)
CC_ALIGN_DATA(32) static uint8_t qspi_status[32];
CC_ALIGN_DATA(32) static uint8_t qspi_bounce[W25QXX_PAGE_SIZE_BYTES];

// 1-line opcode-only command (no address, no data)
static void qspi_cmd_op_only(wspi_command_t *cmd, uint8_t op) {
  cmd->cmd   = op;
  cmd->addr  = 0U;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_NONE |
               WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_NONE |
               WSPI_CFG_CMD_SIZE_8;
}

// 1-line opcode + 1-line data read (jedec, status registers)
static void qspi_cmd_read1(wspi_command_t *cmd, uint8_t op) {
  cmd->cmd   = op;
  cmd->addr  = 0U;
  cmd->alt   = 0U;
  cmd->dummy = 0U;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_NONE |
               WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_ONE_LINE |
               WSPI_CFG_CMD_SIZE_8;
}

bool w25qxx_qspi_lines_need_qe(w25qxx_qspi_lines_t lines) {
  return lines == W25QXX_QSPI_QUAD;
}

void w25qxx_qspi_build_read_cmd(wspi_command_t *cmd,
                                w25qxx_qspi_lines_t lines, uint32_t addr) {
  uint8_t op;
  uint32_t data_mode;
  uint8_t dummy;

  switch (lines) {
    case W25QXX_QSPI_DUAL:
      op        = 0x3BU; // fast read dual output (1-1-2)
      data_mode = WSPI_CFG_DATA_MODE_TWO_LINES;
      dummy     = W25QXX_QSPI_READ_DUMMY;
      break;
    case W25QXX_QSPI_QUAD:
      op        = 0x6BU; // fast read quad output (1-1-4)
      data_mode = WSPI_CFG_DATA_MODE_FOUR_LINES;
      dummy     = W25QXX_QSPI_READ_DUMMY;
      break;
    case W25QXX_QSPI_SINGLE:
    default:
      // 0x03 read data: 0 dummy, chip drives continuously. more robust on a
      // marginal bus than 0x0B fast read (which has a dummy turnaround phase)
      op        = W25QXX_CMD_READ_DATA; // 0x03
      data_mode = WSPI_CFG_DATA_MODE_ONE_LINE;
      dummy     = 0U;
      break;
  }

  cmd->cmd   = op;
  cmd->addr  = addr;
  cmd->alt   = 0U;
  cmd->dummy = dummy;
  cmd->cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
               WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_ALT_MODE_NONE |
               data_mode | WSPI_CFG_CMD_SIZE_8;
}

bool w25qxx_qspi_reset(WSPIDriver *wspi) {
  wspi_command_t cmd;

  qspi_cmd_op_only(&cmd, W25QXX_CMD_RESET_ENABLE); // 0x66
  if (wspiCommand(wspi, &cmd) != MSG_OK) {
    return false;
  }
  qspi_cmd_op_only(&cmd, W25QXX_CMD_RESET_DEVICE); // 0x99
  if (wspiCommand(wspi, &cmd) != MSG_OK) {
    return false;
  }
  chThdSleepMilliseconds(1); // tRST
  return true;
}

bool w25qxx_qspi_read_jedec(WSPIDriver *wspi, uint8_t out[3]) {
  wspi_command_t cmd;

  qspi_cmd_read1(&cmd, W25QXX_CMD_READ_JEDEC_ID); // 0x9F
  if (wspiReceive(wspi, &cmd, 3U, qspi_status) != MSG_OK) {
    return false;
  }
  cacheBufferInvalidate(qspi_status, 3U);
  out[0] = qspi_status[0];
  out[1] = qspi_status[1];
  out[2] = qspi_status[2];
  return true;
}

static bool qspi_read_status(WSPIDriver *wspi, uint8_t op, uint8_t *val) {
  wspi_command_t cmd;

  qspi_cmd_read1(&cmd, op);
  if (wspiReceive(wspi, &cmd, 1U, qspi_status) != MSG_OK) {
    return false;
  }
  cacheBufferInvalidate(qspi_status, sizeof qspi_status);
  *val = qspi_status[0];
  return true;
}

static bool qspi_write_enable(WSPIDriver *wspi) {
  wspi_command_t cmd;

  // send write-enable and confirm WEL actually latched, retrying a couple of
  // times. the first indirect command after leaving memory-mapped mode can be
  // dropped; without this check an erase/program silently no-ops
  for (int attempt = 0; attempt < 3; attempt++) {
    uint8_t sr1;
    qspi_cmd_op_only(&cmd, W25QXX_CMD_WRITE_ENABLE);
    if (wspiCommand(wspi, &cmd) != MSG_OK) {
      return false;
    }
    if (!qspi_read_status(wspi, W25QXX_CMD_READ_STATUS_REG1, &sr1)) {
      return false;
    }
    if (sr1 & W25QXX_STATUS_WEL) {
      return true;
    }
  }
  return false;
}

bool w25qxx_qspi_wait_idle(WSPIDriver *wspi, uint32_t timeout_ms) {
  systime_t deadline = chVTGetSystemTimeX() + TIME_MS2I(timeout_ms);

  do {
    uint8_t sr1;
    if (!qspi_read_status(wspi, W25QXX_CMD_READ_STATUS_REG1, &sr1)) {
      return false;
    }
    if ((sr1 & W25QXX_STATUS_BUSY) == 0U) {
      return true;
    }
    chThdSleepMilliseconds(1);
  } while (chVTGetSystemTimeX() < deadline);
  return false;
}

bool w25qxx_qspi_set_quad_enable(WSPIDriver *wspi) {
  wspi_command_t cmd;
  uint8_t sr2;

  if (!qspi_read_status(wspi, W25QXX_CMD_READ_STATUS_REG2, &sr2)) { // 0x35
    return false;
  }
  if (sr2 & W25Q_SR2_QE_BIT) {
    return true; // already set; QE is non-volatile and persists across resets
  }
  if (!qspi_write_enable(wspi)) {
    return false;
  }
  qspi_status[0] = (uint8_t)(sr2 | W25Q_SR2_QE_BIT);
  cmd.cmd   = 0x31U; // write status register 2 (winbond extension)
  cmd.addr  = 0U;
  cmd.alt   = 0U;
  cmd.dummy = 0U;
  cmd.cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_NONE |
              WSPI_CFG_ALT_MODE_NONE | WSPI_CFG_DATA_MODE_ONE_LINE |
              WSPI_CFG_CMD_SIZE_8;
  cacheBufferFlush(qspi_status, sizeof qspi_status);
  if (wspiSend(wspi, &cmd, 1U, qspi_status) != MSG_OK) {
    return false;
  }
  return w25qxx_qspi_wait_idle(wspi, 1000);
}

bool w25qxx_qspi_erase_sector(WSPIDriver *wspi, uint32_t addr) {
  wspi_command_t cmd;

  if (!qspi_write_enable(wspi)) {
    return false;
  }
  cmd.cmd   = W25QXX_CMD_SECTOR_ERASE; // 0x20
  cmd.addr  = addr;
  cmd.alt   = 0U;
  cmd.dummy = 0U;
  cmd.cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
              WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_ALT_MODE_NONE |
              WSPI_CFG_DATA_MODE_NONE | WSPI_CFG_CMD_SIZE_8;
  if (wspiCommand(wspi, &cmd) != MSG_OK) {
    return false;
  }
  return w25qxx_qspi_wait_idle(wspi, 1000);
}

bool w25qxx_qspi_program_page(WSPIDriver *wspi, uint32_t addr,
                              const uint8_t *buf, size_t len) {
  wspi_command_t cmd;

  if (len == 0U || len > W25QXX_PAGE_SIZE_BYTES) {
    return false;
  }
  if (!qspi_write_enable(wspi)) {
    return false;
  }
  // bounce through an aligned buffer so callers need not align their data
  memcpy(qspi_bounce, buf, len);
  cacheBufferFlush(qspi_bounce, sizeof qspi_bounce);
  cmd.cmd   = W25QXX_CMD_PAGE_PROGRAM; // 0x02
  cmd.addr  = addr;
  cmd.alt   = 0U;
  cmd.dummy = 0U;
  cmd.cfg   = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
              WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_ALT_MODE_NONE |
              WSPI_CFG_DATA_MODE_ONE_LINE | WSPI_CFG_CMD_SIZE_8;
  if (wspiSend(wspi, &cmd, len, qspi_bounce) != MSG_OK) {
    return false;
  }
  return w25qxx_qspi_wait_idle(wspi, 100);
}

bool w25qxx_qspi_read(WSPIDriver *wspi, w25qxx_qspi_lines_t lines,
                      uint32_t addr, uint8_t *buf, size_t len) {
  // read in page-sized chunks through the aligned bounce buffer so callers
  // need not align their destination
  while (len > 0U) {
    size_t chunk = (len > sizeof qspi_bounce) ? sizeof qspi_bounce : len;
    wspi_command_t cmd;

    w25qxx_qspi_build_read_cmd(&cmd, lines, addr);
    if (wspiReceive(wspi, &cmd, chunk, qspi_bounce) != MSG_OK) {
      return false;
    }
    cacheBufferInvalidate(qspi_bounce, sizeof qspi_bounce);
    memcpy(buf, qspi_bounce, chunk);
    addr += chunk;
    buf  += chunk;
    len  -= chunk;
  }
  return true;
}
