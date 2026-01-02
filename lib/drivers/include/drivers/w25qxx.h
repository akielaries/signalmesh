#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "drivers/driver_api.h"
#include "drivers/spi.h"


#ifdef __cplusplus
extern "C" {
#endif

// W25QXX Flash commands
#define W25QXX_CMD_WRITE_ENABLE            0x06
#define W25QXX_CMD_WRITE_DISABLE           0x04
#define W25QXX_CMD_READ_STATUS_REG1        0x05
#define W25QXX_CMD_READ_STATUS_REG2        0x35
#define W25QXX_CMD_READ_STATUS_REG3        0x15
#define W25QXX_CMD_WRITE_STATUS_REG        0x01
#define W25QXX_CMD_PAGE_PROGRAM            0x02
#define W25QXX_CMD_QUAD_PAGE_PROGRAM       0x32
#define W25QXX_CMD_READ_DATA               0x03
#define W25QXX_CMD_FAST_READ               0x0B
#define W25QXX_CMD_FAST_READ_QUAD          0xEB
#define W25QXX_CMD_SECTOR_ERASE            0x20
#define W25QXX_CMD_32KB_BLOCK_ERASE        0x52
#define W25QXX_CMD_64KB_BLOCK_ERASE        0xD8
#define W25QXX_CMD_CHIP_ERASE              0xC7
#define W25QXX_CMD_CHIP_ERASE_ALT          0x60
#define W25QXX_CMD_READ_SFDP               0x5A
#define W25QXX_CMD_ERASE_SECURITY_REG      0x44
#define W25QXX_CMD_PROGRAM_SECURITY_REG    0x42
#define W25QXX_CMD_READ_SECURITY_REG       0x48
#define W25QXX_CMD_GLOBAL_BLOCK_LOCK       0x7E
#define W25QXX_CMD_GLOBAL_BLOCK_UNLOCK     0x98
#define W25QXX_CMD_READ_MANUFACTURER_ID    0x90
#define W25QXX_CMD_READ_UNIQUE_ID          0x4B
#define W25QXX_CMD_READ_JEDEC_ID           0x9F
#define W25QXX_CMD_ENTER_DEEP_POWER_DOWN   0xB9
#define W25QXX_CMD_RELEASE_DEEP_POWER_DOWN 0xAB
#define W25QXX_CMD_RESET_ENABLE            0x66
#define W25QXX_CMD_RESET_DEVICE            0x99

// status register bits
#define W25QXX_STATUS_BUSY 0x01
#define W25QXX_STATUS_WEL  0x02
#define W25QXX_STATUS_BP0  0x04
#define W25QXX_STATUS_BP1  0x08
#define W25QXX_STATUS_BP2  0x10
#define W25QXX_STATUS_TB   0x20
#define W25QXX_STATUS_SEC  0x40
#define W25QXX_STATUS_SRP0 0x80

// device sizes (adjust based on specific variant)
#define W25Q64_SIZE_BYTES  (8 * 1024 * 1024)  // 8MB
#define W25Q128_SIZE_BYTES (16 * 1024 * 1024) // 16MB
#define W25Q32_SIZE_BYTES  (4 * 1024 * 1024)  // 4MB
#define W25Q16_SIZE_BYTES  (2 * 1024 * 1024)  // 2MB

// common page and sector sizes
#define W25QXX_PAGE_SIZE_BYTES    256
#define W25QXX_SECTOR_SIZE_BYTES  (4 * 1024)  // 4KB
#define W25QXX_BLOCK32_SIZE_BYTES (32 * 1024) // 32KB
#define W25QXX_BLOCK64_SIZE_BYTES (64 * 1024) // 64KB

#define W25QXX_MFG_WINBOND_ID 0xEF

// JEDEC ID information
typedef struct {
  uint8_t manufacturer_id;
  uint8_t device_id_high;
  uint8_t device_id_low;
} w25qxx_jedec_id_t;

// W25QXX device private data structure
typedef struct {
  spi_bus_t bus;
  char device_name[16];
  uint32_t size_bytes;
  uint8_t manufacturer_id;
  uint16_t device_id;
} w25qxx_t;

// W25QXX driver variants (based on capacity)
typedef enum {
  W25Q16 = 0,
  W25Q32,
  W25Q64,
  W25Q128,
  W25Q256,
} w25qxx_variant_t;

extern const driver_t w25qxx_driver;

int w25qxx_chip_erase(w25qxx_t *flash_dev);
int w25qxx_sector_erase(w25qxx_t *flash_dev, uint32_t sector_addr);
int w25qxx_block_erase(w25qxx_t *flash_dev, uint32_t block_addr, uint32_t block_size);

#ifdef __cplusplus
}
#endif
