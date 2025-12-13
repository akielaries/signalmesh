#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct device device_t;
typedef struct driver driver_t;

/* Common error codes */
#define DRIVER_OK        0
#define DRIVER_ERROR    -1
#define DRIVER_INVALID_PARAM -2
#define DRIVER_NOT_FOUND -3
#define DRV_OK        0
#define DRV_EINVAL   -1
#define DRV_EIO       -2
#define DRV_ENOTSUP  -3

/* Device instance */
struct device {
  const char  *name;
  const driver_t *driver;
  void *bus;     /* I2C, SPI, etc */
  void *priv;    /* driver private data */
  bool is_active;
};

/* Driver descriptor */
struct driver {
  const char *name;
  int (*init)(device_t *dev);
  int (*probe)(device_t *dev);
  int (*remove)(device_t *dev);
  int (*ioctl)(device_t *dev, uint32_t cmd, void *arg);
  int (*poll)(device_t *dev);
};

