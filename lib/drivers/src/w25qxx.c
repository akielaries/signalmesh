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

static int w25qxx_init(device_t *dev) {
  if (dev->priv == NULL) {
    return DRIVER_ERROR;
  }
  bsp_printf("initializing W25QXX flash\n");
  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;

  // assign the SPI bus configuration
  flash_dev->bus = *(spi_bus_t *)dev->bus;

  // release from deep power down
  dma_buffer[0] = W25QXX_CMD_RELEASE_DEEP_POWER_DOWN;
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 1);
  spi_bus_release(&flash_dev->bus);

  /*
  chThdSleepMicroseconds(30); // wait tRES1 (min 30us)

  // read JEDEC ID to identify the device
  w25qxx_jedec_id_t jedec_id;
  if (w25qxx_read_jedec_id(flash_dev, &jedec_id) != DRIVER_OK) {
    return DRIVER_ERROR;
  }

  flash_dev->manufacturer_id = jedec_id.manufacturer_id;
  flash_dev->device_id       = (jedec_id.device_id_high << 8) | jedec_id.device_id_low;
  */

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
      break;
    case 0x4018: // W25Q128
      flash_dev->size_bytes = W25Q128_SIZE_BYTES;
      break;
    default:
      flash_dev->size_bytes = W25Q64_SIZE_BYTES; // default to 8MB if unknown
      break;
  }

  bsp_printf("mfg id: 0x%X\n", flash_dev->manufacturer_id);
  bsp_printf("dev id: 0x%X\n", flash_dev->device_id);

  return DRIVER_OK;
}

static int32_t w25qxx_read(device_t *dev, uint32_t offset, void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;

  w25qxx_t *flash_dev = (w25qxx_t *)dev->priv;
  if (offset + count > flash_dev->size_bytes)
    return DRIVER_INVALID_PARAM;

  // wait for device to be ready
  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
    return DRIVER_ERROR;

  // build read command with 24-bit address in the DMA-safe buffer
  dma_buffer[0] = W25QXX_CMD_READ_DATA;
  dma_buffer[1] = (uint8_t)(offset >> 16);
  dma_buffer[2] = (uint8_t)(offset >> 8);
  dma_buffer[3] = (uint8_t)(offset & 0xFF);

  // transmit command and address, then receive data
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 4);
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
    if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;

    if (w25qxx_write_enable(flash_dev) != DRIVER_OK)
      return bytes_written > 0 ? bytes_written : DRIVER_ERROR;

    uint32_t current_addr    = offset + bytes_written;
    uint32_t page_offset     = current_addr % W25QXX_PAGE_SIZE_BYTES;
    uint32_t page_space      = W25QXX_PAGE_SIZE_BYTES - page_offset;
    uint32_t bytes_remaining = count - bytes_written;
    uint32_t chunk_size      = (page_space < bytes_remaining) ? page_space : bytes_remaining;

    // build page program command in the DMA-safe buffer
    dma_buffer[0] = W25QXX_CMD_PAGE_PROGRAM;
    dma_buffer[1] = (uint8_t)(current_addr >> 16);
    dma_buffer[2] = (uint8_t)(current_addr >> 8);
    dma_buffer[3] = (uint8_t)(current_addr & 0xFF);

    // transmit command header, then the user data directly
    spi_bus_acquire(&flash_dev->bus);
    spi_bus_send(&flash_dev->bus, dma_buffer, 4);
    spi_bus_send(&flash_dev->bus, &data_ptr[bytes_written], chunk_size);
    spi_bus_release(&flash_dev->bus);

    bytes_written += chunk_size;
  }

  // wait for the final write to complete
  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK)
    return bytes_written > 0 ? bytes_written : DRIVER_ERROR;

  return bytes_written;
}

// helper function implementations
static int w25qxx_wait_busy(w25qxx_t *flash_dev) {
  uint32_t timeout    = 100000; // 100 second timeout
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
  dma_buffer[0] = W25QXX_CMD_WRITE_ENABLE;

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 1);
  spi_bus_release(&flash_dev->bus);

  chThdSleepMicroseconds(10);

  // verify write enable bit is set
  if ((w25qxx_read_status_register(flash_dev, 1) & W25QXX_STATUS_WEL) == 0) {
    bsp_printf("W25QXX: Failed to enable write (WEL not set).\n");
    return DRIVER_ERROR;
  }

  return DRIVER_OK;
}

static int w25qxx_write_disable(w25qxx_t *flash_dev) {
  dma_buffer[0] = W25QXX_CMD_WRITE_DISABLE;

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 1);
  spi_bus_release(&flash_dev->bus);

  return DRIVER_OK;
}

static uint8_t w25qxx_read_status_register(w25qxx_t *flash_dev, uint8_t reg_num) {
  switch (reg_num) {
    case 1:
      dma_buffer[0] = W25QXX_CMD_READ_STATUS_REG1;
      break;
    case 2:
      dma_buffer[0] = W25QXX_CMD_READ_STATUS_REG2;
      break;
    case 3:
      dma_buffer[0] = W25QXX_CMD_READ_STATUS_REG3;
      break;
    default:
      return 0xFF; // error indication
  }

  dma_buffer[1] = 0xFF; // dummy byte

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_exchange(&flash_dev->bus, dma_buffer, dma_buffer, 2);
  spi_bus_release(&flash_dev->bus);

  // the first received byte is garbage, status is the second byte
  return dma_buffer[1];
}

static int w25qxx_read_jedec_id(w25qxx_t *flash_dev, w25qxx_jedec_id_t *id) {
  // command plus 3 dummy bytes to clock out the 3-byte ID
  dma_buffer[0] = W25QXX_CMD_READ_JEDEC_ID;
  dma_buffer[1] = 0xFF; // dummy
  dma_buffer[2] = 0xFF; // dummy
  dma_buffer[3] = 0xFF; // dummy

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_exchange(&flash_dev->bus, dma_buffer, dma_buffer, 4);
  spi_bus_release(&flash_dev->bus);

  // the first received byte is garbage, ID is in bytes 2, 3, 4
  id->manufacturer_id = dma_buffer[1];
  id->device_id_high  = dma_buffer[2];
  id->device_id_low   = dma_buffer[3];

  return DRIVER_OK;
}

int w25qxx_chip_erase(w25qxx_t *flash_dev) {
  if (w25qxx_write_enable(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Chip erase write enable failed.\n");
    return DRIVER_ERROR;
  }

  dma_buffer[0] = W25QXX_CMD_CHIP_ERASE;
  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 1);
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

  dma_buffer[0] = W25QXX_CMD_SECTOR_ERASE;
  dma_buffer[1] = (uint8_t)(sector_addr >> 16);
  dma_buffer[2] = (uint8_t)(sector_addr >> 8);
  dma_buffer[3] = (uint8_t)(sector_addr & 0xFF);

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 4);
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

  spi_bus_acquire(&flash_dev->bus);
  spi_bus_send(&flash_dev->bus, dma_buffer, 4);
  spi_bus_release(&flash_dev->bus);

  if (w25qxx_wait_busy(flash_dev) != DRIVER_OK) {
    bsp_printf("W25QXX: Block erase busy wait timeout.\n");
    return DRIVER_ERROR;
  }
  return DRIVER_OK;
}
