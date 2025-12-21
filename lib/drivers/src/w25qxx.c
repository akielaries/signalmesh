#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/w25qxx.h"
#include "ch.h"
#include "hal.h"
#include "drivers/spi.h" // Include for spi_bus_acquire/release etc.

// Forward declarations for driver_t interface
static int w25qxx_init(device_t *dev);
static int32_t w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count);
static int32_t w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count);

// Forward declarations for internal helper functions and erase operations
static int w25qxx_wait_busy(w25qxx_t *flash_dev);
static int w25qxx_write_enable(w25qxx_t *flash_dev);
static int w25qxx_write_disable(w25qxx_t *flash_dev);
static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num);
static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id);

// Erase functions (exposed in w25qxx.h)
int w25qxx_chip_erase(w25qxx_t *flash_dev);
int w25qxx_sector_erase(w25qxx_t *flash_dev, uint32_t sector_addr);
int w25qxx_block_erase(w25qxx_t *flash_dev, uint32_t block_addr, uint32_t block_size);

// Driver descriptor
const driver_t w25qxx_driver __attribute__((used)) = {
  .name               = "w25qxx",
  .init               = w25qxx_init,
  .read               = w25qxx_read,
  .write              = w25qxx_write,
  .poll               = NULL,
  .readings_directory = NULL,
};

static int w25qxx_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("W25QXX init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;

  bsp_printf("W25QXX: Initializing...\n");

  chThdSleepMilliseconds(10);

  bsp_printf("W25QXX: dev->bus address: %p\n", dev->bus);
  // Assign the SPI bus configuration
  flash_dev->bus = *(spi_bus_t *)dev->bus;
  bsp_printf("W25QXX: flash_dev->bus assigned. spi_driver: %p, spi_config: %p\n",
             flash_dev->bus.spi_driver, flash_dev->bus.spi_config);
  
  // Release from deep power down if needed
  uint8_t cmd_release = W25QXX_CMD_RELEASE_DEEP_POWER_DOWN;
  bsp_printf("W25QXX: Releasing deep power down...\n");
  bsp_printf("W25QXX: Acquiring SPI bus...\n");
  spi_bus_acquire(&flash_dev->bus);
  bsp_printf("W25QXX: SPI bus acquired. Sending command...\n");

  chThdSleepMilliseconds(10);
  spi_bus_send(&flash_dev->bus, &cmd_release, 1);
  chThdSleepMilliseconds(10);
  bsp_printf("W25QXX: Command sent. Releasing SPI bus...\n");
  spi_bus_release(&flash_dev->bus);
  bsp_printf("W25QXX: SPI bus released.\n");
  chThdSleepMicroseconds(3); // tRDPD = 3us
  bsp_printf("W25QXX: Deep power down released.\n");

  // Read JEDEC ID to identify the device
  w25qxx_jedec_id_t jedec_id;
  if (w25qxx_read_jedec_id(flash_dev, &jedec_id) != DRIVER_OK) {
    bsp_printf("W25QXX: Failed to read JEDEC ID.\n");
    return DRIVER_ERROR;
  }

  flash_dev->manufacturer_id = jedec_id.manufacturer_id;
  flash_dev->device_id       = (jedec_id.device_id_high << 8) | jedec_id.device_id_low;

  // Determine device size based on JEDEC ID
  switch (flash_dev->device_id) {
    case 0x4015: // W25Q16
      flash_dev->size_bytes = W25Q16_SIZE_BYTES;
      break;
    case 0x4016: // W25Q32
      flash_dev->size_bytes = W25Q32_SIZE_BYTES;
      break;
    case 0x4017: // W25Q64
      flash_dev->size_bytes = W25Q64_SIZE_BYTES;
      break;
    case 0x4018: // W25Q128
      flash_dev->size_bytes = W25Q128_SIZE_BYTES;
      break;
    default:
      bsp_printf("W25QXX: Unknown device ID 0x%04X\n", flash_dev->device_id);
      flash_dev->size_bytes = W25Q64_SIZE_BYTES; // Default to 8MB if unknown
      break;
  }

  bsp_printf("W25QXX: Found device with JEDEC ID: 0x%02X%02X%02X (%luKB)\n",
             jedec_id.manufacturer_id,
             jedec_id.device_id_high,
             jedec_id.device_id_low,
             flash_dev->size_bytes / 1024);

  bsp_printf("W25QXX: Initialization complete.\n");
  return DRIVER_OK;
}

static int32_t w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes)
    return DRIVER_INVALID_PARAM;

  // Wait for device to be ready
  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
    return DRIVER_ERROR;

  // Build read command with 24-bit address
  uint8_t cmd[4];
  cmd[0] = W25QXX_CMD_READ_DATA;
  cmd[1] = (uint8_t)(offset >> 16);
  cmd[2] = (uint8_t)(offset >> 8);
  cmd[3] = (uint8_t)(offset & 0xFF);

  // Transmit command and address, then receive data
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, cmd, 4);
  spi_bus_receive(&flash_dev->bus, (uint8_t *)buf, count);
  spi_bus_release(&flash_dev->bus);

  return count;
}

static int32_t w25qxx_write(device_t *dev, uint32_t offset, const void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes)
    return DRIVER_INVALID_PARAM;

  const uint8_t *data_ptr = (const uint8_t *)buf;
  size_t bytes_written    = 0;

  while (bytes_written < count) {
    // Wait for device to be ready
    if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;

    // Enable writing
    if (w25qxx_write_enable(flash_dev) != DRIVER_OK)
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;

    uint32_t current_addr    = offset + bytes_written;
    uint32_t page_offset     = current_addr % W25QXX_PAGE_SIZE_BYTES;
    uint32_t page_space      = W25QXX_PAGE_SIZE_BYTES - page_offset;
    uint32_t bytes_remaining = count - bytes_written;
    uint32_t chunk_size      = (page_space < bytes_remaining) ? page_space : bytes_remaining;

    // Build page program command with 24-bit address
    uint8_t cmd[4 + chunk_size];
    cmd[0] = W25QXX_CMD_PAGE_PROGRAM;
    cmd[1] = (uint8_t)(current_addr >> 16);
    cmd[2] = (uint8_t)(current_addr >> 8);
    cmd[3] = (uint8_t)(current_addr & 0xFF);
    memcpy(&cmd[4], &data_ptr[bytes_written], chunk_size);

    // Transmit page program command and data
    spi_bus_acquire(&flash_dev->bus);
    spi_bus_send(&flash_dev->bus, cmd, 4 + chunk_size);
    spi_bus_release(&flash_dev->bus);

    bytes_written += chunk_size;

    // Wait for write to complete
    if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;
  }

  return bytes_written;
}

// Helper function implementations
static int w25qxx_wait_busy(w25qxx_t *flash_dev) {
  uint32_t timeout    = 1000; // 1 second timeout
  uint32_t start_time = chVTGetSystemTimeX();

  while ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_BUSY) != 0) {
    if ((chVTGetSystemTimeX() - start_time) > TIME_MS2I(timeout)) {
      bsp_printf("W25QXX: Wait busy timeout\n");
      return DRIVER_ERROR;
    }
    chThdSleepMicroseconds(10);
  }

  return DRIVER_OK;
}

static int w25qxx_write_enable(w25qxx_t *flash_dev) {
  uint8_t cmd = W25QXX_CMD_WRITE_ENABLE;
  
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, &cmd, 1);
  spi_bus_release(&flash_dev->bus);

  // Verify write enable bit is set
  if ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_WEL) == 0) {
    bsp_printf("W25QXX: Failed to enable write (WEL not set).\n");
    return DRIVER_ERROR;
  }

  return DRIVER_OK;
}

static int w25qxx_write_disable(w25qxx_t *flash_dev) {
  uint8_t cmd = W25QXX_CMD_WRITE_DISABLE;
  
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, &cmd, 1);
  spi_bus_release(&flash_dev->bus);
  
  return DRIVER_OK;
}

static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num) {
  uint8_t cmd;
  uint8_t status;

  switch (reg_num) {
    case 1:
      cmd = W25QXX_CMD_READ_STATUS_REG1;
      break;
    case 2:
      cmd = W25QXX_CMD_READ_STATUS_REG2;
      break;
    case 3:
      cmd = W25QXX_CMD_READ_STATUS_REG3;
      break;
    default:
      bsp_printf("W25QXX: Invalid status register number: %d\n", reg_num);
      return 0xFF; // Error indication
  }

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, &cmd, 1);
  spi_bus_receive(&flash_dev->bus, &status, 1);
  spi_bus_release(&flash_dev->bus);
  
  return status;
}

static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id) {
  uint8_t cmd = W25QXX_CMD_READ_JEDEC_ID;
  uint8_t response[3];

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, &cmd, 1);
  spi_bus_receive(&flash_dev->bus, response, 3);
  spi_bus_release(&flash_dev->bus);
  
  id->manufacturer_id = response[0];
  id->device_id_high  = response[1];
  id->device_id_low   = response[2];

  return DRIVER_OK;
}

int w25qxx_chip_erase(w25qxx_t *flash_dev) {
  if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Chip erase write enable failed.\n");
    return DRIVER_ERROR;
  }

  uint8_t cmd = W25QXX_CMD_CHIP_ERASE;
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, &cmd, 1);
  spi_bus_release(&flash_dev->bus);
  
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

  uint8_t cmd[4];
  cmd[0] = W25QXX_CMD_SECTOR_ERASE;
  cmd[1] = (uint8_t)(sector_addr >> 16);
  cmd[2] = (uint8_t)(sector_addr >> 8);
  cmd[3] = (uint8_t)(sector_addr & 0xFF);

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, cmd, 4);
  spi_bus_release(&flash_dev->bus);

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

  uint8_t cmd[4];
  if (block_size == W25QXX_BLOCK32_SIZE_BYTES) {
    cmd[0] = W25QXX_CMD_32KB_BLOCK_ERASE;
  } else if (block_size == W25QXX_BLOCK64_SIZE_BYTES) {
    cmd[0] = W25QXX_CMD_64KB_BLOCK_ERASE;
  } else {
    bsp_printf("W25QXX: Invalid block size for erase: %lu\n", (unsigned long)block_size);
    return DRIVER_INVALID_PARAM;
  }
  
  cmd[1] = (uint8_t)(block_addr >> 16);
  cmd[2] = (uint8_t)(block_addr >> 8);
  cmd[3] = (uint8_t)(block_addr & 0xFF);

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, cmd, 4);
  spi_bus_release(&flash_dev->bus);

  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Block erase busy wait timeout.\n");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}
