#include <string.h>
#include "bsp/utils/bsp_io.h"
#include "drivers/24lc256.h"
#include "ch.h"
#include "hal.h"
#include "drivers/i2c.h"

// Forward declarations
static int eeprom_24lc256_init(device_t *dev);
static int32_t eeprom_24lc256_read(device_t *dev, uint32_t offset, void *buf, size_t count);
static int32_t eeprom_24lc256_write(device_t *dev, uint32_t offset, const void *buf, size_t count);

const driver_t eeprom_24lc256_driver __attribute__((used)) = {
  .name               = "24lc256",
  .init               = eeprom_24lc256_init,
  .read               = eeprom_24lc256_read,
  .write              = eeprom_24lc256_write,
  .poll               = NULL,
  .readings_directory = NULL,
};

static int eeprom_24lc256_init(device_t *dev) {
  if (dev->priv == NULL) {
    bsp_printf("24LC256 init error: private data not allocated\n");
    return DRIVER_ERROR;
  }
  eeprom_24lc256_t *eeprom_dev = (eeprom_24lc256_t *)dev->priv;

  bsp_printf("24LC256: Initializing...\n");

  eeprom_dev->bus.i2c  = (I2CDriver *)dev->bus;
  eeprom_dev->bus.addr = EEPROM_24LC256_DEFAULT_ADDRESS;

  // Check if device is responsive
  uint8_t dummy;
  msg_t ret = i2c_master_receive(eeprom_dev->bus.i2c, eeprom_dev->bus.addr, &dummy, 1);
  if (ret != MSG_OK) {
    bsp_printf("24LC256: Device not responding.\n");
    i2c_handle_error(eeprom_dev->bus.i2c, "24lc256_init");
    return DRIVER_ERROR;
  }

  bsp_printf("24LC256: Initialization complete.\n");
  return DRIVER_OK;
}

static int32_t eeprom_24lc256_read(device_t *dev, uint32_t offset, void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;
  if (offset + count > EEPROM_24LC256_SIZE_BYTES)
    return DRIVER_INVALID_PARAM;

  eeprom_24lc256_t *eeprom_dev = (eeprom_24lc256_t *)dev->priv;

  uint8_t addr_buf[2];
  addr_buf[0] = (uint8_t)(offset >> 8);
  addr_buf[1] = (uint8_t)(offset & 0xFF);

  msg_t ret =
    i2c_master_transmit(eeprom_dev->bus.i2c, eeprom_dev->bus.addr, addr_buf, 2, buf, count);

  if (ret != MSG_OK) {
    i2c_handle_error(eeprom_dev->bus.i2c, "24lc256_read");
    return -1;
  }
  return count;
}

static int32_t eeprom_24lc256_write(device_t *dev, uint32_t offset, const void *buf, size_t count) {
  if (dev == NULL || buf == NULL)
    return DRIVER_INVALID_PARAM;
  if (offset + count > EEPROM_24LC256_SIZE_BYTES)
    return DRIVER_INVALID_PARAM;

  eeprom_24lc256_t *eeprom_dev = (eeprom_24lc256_t *)dev->priv;
  const uint8_t *data_ptr      = (const uint8_t *)buf;
  size_t bytes_written         = 0;

  while (bytes_written < count) {
    // Wait until EEPROM is ready (finishes internal write cycle)
    while (i2c_master_transmit(eeprom_dev->bus.i2c, eeprom_dev->bus.addr, NULL, 0, NULL, 0) !=
           MSG_OK) {
      chThdSleepMilliseconds(1);
    }

    uint16_t current_addr = offset + bytes_written;
    uint16_t bytes_to_write_on_page =
      EEPROM_24LC256_PAGE_SIZE_BYTES - (current_addr % EEPROM_24LC256_PAGE_SIZE_BYTES);
    uint16_t bytes_remaining_in_buffer = count - bytes_written;
    uint16_t chunk_size                = (bytes_to_write_on_page < bytes_remaining_in_buffer)
                                           ? bytes_to_write_on_page
                                           : bytes_remaining_in_buffer;

    uint8_t tx_buf[2 + chunk_size];
    tx_buf[0] = (uint8_t)(current_addr >> 8);
    tx_buf[1] = (uint8_t)(current_addr & 0xFF);
    memcpy(&tx_buf[2], &data_ptr[bytes_written], chunk_size);

    msg_t ret = i2c_master_transmit(eeprom_dev->bus.i2c,
                                    eeprom_dev->bus.addr,
                                    tx_buf,
                                    sizeof(tx_buf),
                                    NULL,
                                    0);
    if (ret != MSG_OK) {
      i2c_handle_error(eeprom_dev->bus.i2c, "24lc256_write");
      return bytes_written > 0 ? bytes_written : -1;
    }

    bytes_written += chunk_size;
    // 5msec write cycle according to data sheet...
    chThdSleepMilliseconds(5); // Wait for the write cycle t_WR
  }

  return bytes_written;
}
