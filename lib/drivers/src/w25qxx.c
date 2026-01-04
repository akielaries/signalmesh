#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/w25qxx.h"
#include "ch.h"
#include "hal.h"
#include "drivers/spi.h" // include for spi_bus_acquire/release etc.

CC_ALIGN_DATA(32) static uint8_t dma_buffer[32];

// forward declarations for driver_t interface
static int w25qxx_init(device_t *dev);
static int32_t w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count);
static int32_t w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count);

// forward declarations for internal helper functions and erase operations
static int w25qxx_wait_busy(w25qxx_t *flash_dev);
static int w25qxx_write_enable(w25qxx_t *flash_dev);
static int w25qxx_write_disable(w25qxx_t *flash_dev);
static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num);
static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id);

// erase functions (exposed in w25qxx.h)
int w25qxx_chip_erase(w25qxx_t *flash_dev);
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

static int32_t w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes)
    return DRIVER_INVALID_PARAM;

  // As requested, w25qxx_wait_busy is not called for read operations.

  // Define a static, DMA-safe buffer for chunked reading.
  // Using a size that can hold a full page plus the 5-byte command header.
  #define READ_CHUNK_SIZE W25QXX_PAGE_SIZE_BYTES
  CC_ALIGN_DATA(32) static uint8_t tx_buffer[5 + READ_CHUNK_SIZE];
  CC_ALIGN_DATA(32) static uint8_t rx_buffer[5 + READ_CHUNK_SIZE];

  size_t bytes_read = 0;
  uint8_t *data_ptr = (uint8_t *)buf;

  while (bytes_read < count) {
    size_t bytes_remaining = count - bytes_read;
    size_t chunk_size = (bytes_remaining < READ_CHUNK_SIZE) ? bytes_remaining : READ_CHUNK_SIZE;
    uint32_t current_addr = offset + bytes_read;

    tx_buffer[0] = W25QXX_CMD_FAST_READ;
    tx_buffer[1] = (uint8_t)(current_addr >> 24) & 0xFF;
    tx_buffer[2] = (uint8_t)(current_addr >> 16) & 0xFF;
    tx_buffer[3] = (uint8_t)(current_addr >> 8) & 0xFF;
    tx_buffer[4] = (uint8_t)(current_addr >> 0) & 0xFF;
    tx_buffer[5] = 0x00; // Dummy byte for Fast Read.

    // Perform the exchange for this chunk. The total length is 5 header bytes
    // plus the number of data bytes to read.
    spi_bus_exchange(&flash_dev->bus, tx_buffer, rx_buffer, 5 + chunk_size);

    // Copy the received data into the user's buffer. The actual data starts
    // at index 5 of the receive buffer, after the garbage bytes from the
    // 5-byte command/address/dummy phase.
    memcpy(data_ptr + bytes_read, rx_buffer + 5, chunk_size);

    bytes_read += chunk_size;
  }

  return bytes_read;
}

static int32_t w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count) {
  if (dev == NULL || buf == NULL) {
    return DRIVER_INVALID_PARAM;
  }

  // A single, DMA-safe buffer for the entire transaction (cmd + address + data)
  // Max size is 4 bytes for command + 256 bytes for a full page write.
  CC_ALIGN_DATA(32) static uint8_t tx_buffer[5 + W25QXX_PAGE_SIZE_BYTES];
  CC_ALIGN_DATA(32) static uint8_t reply_buffer[5 + W25QXX_PAGE_SIZE_BYTES];

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes)
    return DRIVER_INVALID_PARAM;

  const uint8_t *data_ptr = (const uint8_t *)buf;
  size_t bytes_written    = 0;

  while (bytes_written < count) {    // Wait for the chip to be ready before starting a new write cycle.
    // Enable writing for this operation.
    if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;
    }

    uint32_t current_addr    = offset + bytes_written;
    uint32_t page_offset     = current_addr % W25QXX_PAGE_SIZE_BYTES;
    uint32_t page_space      = W25QXX_PAGE_SIZE_BYTES - page_offset;
    uint32_t bytes_remaining = count - bytes_written;
    uint32_t chunk_size      = (page_space < bytes_remaining) ? page_space : bytes_remaining;

    // Build the command and address in the static buffer.
    tx_buffer[0] = W25QXX_CMD_PAGE_PROGRAM;
    tx_buffer[1] = (uint8_t)(current_addr >> 24) & 0xFF;
    tx_buffer[2] = (uint8_t)(current_addr >> 16) & 0xFF;
    tx_buffer[3] = (uint8_t)(current_addr >> 8) & 0xFF;
    tx_buffer[4] = (uint8_t)(current_addr >> 0) & 0xFF;

    // Copy the data chunk into the buffer right after the command.
    memcpy(tx_buffer + 5, data_ptr + bytes_written, chunk_size);

    // Transmit command and data in a single exchange.
    spi_bus_exchange(&flash_dev->bus, tx_buffer, reply_buffer, 4 + chunk_size);
    bsp_printf("wrote bytes\n");

    bytes_written += chunk_size;

    //chThdSleepMicroseconds(50);

    if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
      // If the wait times out, return the number of bytes that were
      // successfully written before the failure.
      return bytes_written;
    }
  }

  return bytes_written;
}

// helper function implementations
static int w25qxx_wait_busy(w25qxx_t *flash_dev) {
  // A long timeout is used here because erase operations can take seconds.
  uint32_t timeout_ms = 10000; // 10 second timeout
  systime_t start_time = chVTGetSystemTimeX();

  bsp_printf("busy wait...\n");

  while ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_BUSY) != 0) {
    if ((chVTGetSystemTimeX() - start_time) > TIME_MS2I(timeout_ms)) {
      bsp_printf("W25QXX: Wait busy timeout\n");
      return DRIVER_ERROR;
    }
    chThdSleepMicroseconds(100); // Sleep for a short duration before polling again
  }

  bsp_printf("busy wait...done\n");
  return DRIVER_OK;
}

static int w25qxx_write_enable(w25qxx_t *flash_dev) {
  CC_ALIGN_DATA(32) static uint8_t cmd[4];
  CC_ALIGN_DATA(32) static uint8_t reply[4];
  cmd[0] = W25QXX_CMD_WRITE_ENABLE;

  spi_bus_exchange(&flash_dev->bus, cmd, reply, 4);

  chThdSleepMicroseconds(10);

  /*
  // verify write enable bit is set
  if ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_WEL) == 0) {
    bsp_printf("W25QXX: Failed to enable write (WEL not set).\n");
    return DRIVER_ERROR;
  }
  */

  bsp_printf("write enabled\n");

  return DRIVER_OK;
}

static int w25qxx_write_disable(w25qxx_t *flash_dev) {
  dma_buffer[0] = W25QXX_CMD_WRITE_DISABLE;

  spi_bus_exchange(&flash_dev->bus, dma_buffer, NULL, 1);

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

  bsp_printf("status: 0x%X 0x%X\n", rx_buf[0], rx_buf[1]);

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

int w25qxx_chip_erase(w25qxx_t *flash_dev) {
  if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Chip erase write enable failed.\n");
    return DRIVER_ERROR;
  }

  dma_buffer[0] = W25QXX_CMD_CHIP_ERASE;
  spi_bus_exchange(&flash_dev->bus, dma_buffer, NULL, 1);

  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Chip erase busy wait timeout.\n");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

int w25qxx_sector_erase(w25qxx_t *flash_dev, uint32_t sector_addr) {
  if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Sector erase write enable failed.\n");
    return DRIVER_ERROR;
  }

  dma_buffer[0] = W25QXX_CMD_SECTOR_ERASE;
  dma_buffer[1] = (uint8_t)(sector_addr >> 16);
  dma_buffer[2] = (uint8_t)(sector_addr >> 8);
  dma_buffer[3] = (uint8_t)(sector_addr & 0xFF);
  spi_bus_exchange(&flash_dev->bus, dma_buffer, NULL, 4);

  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Sector erase busy wait timeout.\n");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}

int w25qxx_block_erase(w25qxx_t *flash_dev, uint32_t block_addr, uint32_t block_size) {
  if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Block erase write enable failed.\n");
    return DRIVER_ERROR;
  }

  if (block_size == W25QXX_BLOCK32_SIZE_BYTES) {
    dma_buffer[0] = W25QXX_CMD_32KB_BLOCK_ERASE;
  } else if (block_size == W25QXX_BLOCK64_SIZE_BYTES) {
    dma_buffer[0] = W25QXX_CMD_64KB_BLOCK_ERASE;
  } else {
    bsp_printf("W25QXX: Invalid block size for erase: %lu\n", (unsigned long)block_size);
    return DRIVER_INVALID_PARAM;
  }

  dma_buffer[1] = (uint8_t)(block_addr >> 16);
  dma_buffer[2] = (uint8_t)(block_addr >> 8);
  dma_buffer[3] = (uint8_t)(block_addr & 0xFF);

  spi_bus_exchange(&flash_dev->bus, dma_buffer, NULL, 4);

  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Block erase busy wait timeout.\n");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}
