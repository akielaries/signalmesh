/**
 * @file driver_api.h
 * @brief Common driver interface and device management API
 *
 * This file defines the generic driver framework used by all device drivers
 * in the system. It provides a unified interface for device initialization,
 * configuration, and operation regardless of the underlying bus type (I2C, SPI,
 * etc).
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#include "driver_readings.h"


/**
 * @brief Forward declaration of device structure
 */
typedef struct device device_t;

/**
 * @brief Forward declaration of driver structure
 */
typedef struct driver driver_t;

/* Common error codes */
/** @brief Operation completed successfully */
#define DRIVER_OK 0
/** @brief General error occurred */
#define DRIVER_ERROR -1
/** @brief Invalid parameter provided */
#define DRIVER_INVALID_PARAM -2
/** @brief Device or driver not found */
#define DRIVER_NOT_FOUND -3

// TODO: these could be more descriptive and live in an enum
/** @brief Operation completed successfully (alias) */
#define DRV_OK 0
/** @brief Invalid argument (alias) */
#define DRV_EINVAL -1
/** @brief Input/output error (alias) */
#define DRV_EIO -2
/** @brief Operation not supported (alias) */
#define DRV_ENOTSUP -3

/**
 * @brief Device instance structure
 *
 * Represents a single device instance in the system. Each device is
 * associated with a specific driver and bus interface.
 */
struct device {
  /** @brief Human-readable device name */
  const char *name;

  /** @brief Pointer to the driver that manages this device */
  const driver_t *driver;

  /** @brief Bus-specific interface (I2C, SPI, etc) */
  void *bus;

  /** @brief Driver-specific private data */
  void *priv;

  /** @brief Device activation status */
  bool is_active;
};

/** @brief Typedef for device ID, using device_t pointer */
typedef device_t *device_id_t;

/**
 * @brief Driver descriptor structure
 *
 * Defines the interface for a device driver. Each driver implements
 * these functions to manage devices of a specific type.
 */
struct driver {
  /** @brief Human-readable driver name */
  const char *name;

  /** @brief Size of the driver's private data structure (context) */
  size_t num_data_bytes;

  /**
   * @brief Initialize the device
   * @param dev Pointer to device structure
   * @return DRIVER_OK on success, error code on failure
   */
  int (*init)(device_t *dev);

  /**
   * @brief Remove device and clean up resources
   * @param dev Pointer to device structure
   * @return DRIVER_OK on success, error code on failure
   */
  int (*remove)(device_t *dev);

  /**
   * @brief Perform device-specific I/O control
   * @param dev Pointer to device structure
   * @param cmd Command identifier
   * @param arg Command-specific argument
   * @return DRIVER_OK on success, error code on failure
   */
  int (*ioctl)(device_t *dev, uint32_t cmd, void *arg);

  /**
   * @brief Function called when a device is being polled
   * @param device_id The ID of the device being polled
   * @param num_readings The number of readings to retrieve
   * @param readings Pointer to an array of driver_reading_t to fill
   * @return DRIVER_OK on success, error code on failure
   */
  int (*poll)(device_id_t device_id, uint32_t num_readings, driver_reading_t *readings);

  /**
   * @brief Read data from a storage device
   * @param dev Pointer to device structure
   * @param offset The offset to start reading from
   * @param buf The buffer to read data into
   * @param count The number of bytes to read
   * @return The number of bytes read, or a negative error code
   */
  int32_t (*read)(device_t *dev, uint32_t offset, void *buf, size_t count);

  /**
   * @brief Write data to a storage device
   * @param dev Pointer to device structure
   * @param offset The offset to start writing to
   * @param buf The buffer to write data from
   * @param count The number of bytes to write
   * @return The number of bytes written, or a negative error code
   */
  int32_t (*write)(device_t *dev, uint32_t offset, const void *buf, size_t count);

  /** @brief Directory of readings provided by this driver */
  const driver_readings_directory_t *readings_directory;
};
