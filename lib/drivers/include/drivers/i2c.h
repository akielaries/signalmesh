/**
 * @file i2c.h
 * @brief Generic I2C bus abstraction layer
 *
 * This file provides a hardware-agnostic interface for I2C communication.
 * It wraps the ChibiOS I2C driver with additional error handling
 * and convenience functions for register-based devices.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include "hal.h"

/**
 * @brief I2C bus configuration structure
 *
 * Encapsulates all information needed to communicate with an I2C device.
 * This provides a clean interface between device drivers and the underlying
 * ChibiOS I2C implementation.
 */
typedef struct {
  /** @brief Pointer to ChibiOS I2C driver instance */
  I2CDriver *i2c;

  /** @brief 7-bit I2C device address */
  uint8_t addr;

  /** @brief Communication timeout in system ticks */
  systime_t timeout;
} i2c_bus_t;

/**
 * @brief Perform I2C master transmission
 *
 * Transmits data to an I2C device and optionally receives data in the same
 * transaction. This is the core I2C communication function.
 *
 * @param i2cp Pointer to I2C driver instance
 * @param addr 7-bit I2C device address
 * @param txbuf Pointer to transmit buffer
 * @param txbytes Number of bytes to transmit
 * @param rxbuf Pointer to receive buffer (can be NULL for write-only)
 * @param rxbytes Number of bytes to receive
 * @return MSG_OK on success, error code on failure
 */
msg_t i2c_master_transmit(I2CDriver *i2cp,
                          i2caddr_t addr,
                          const uint8_t *txbuf,
                          size_t txbytes,
                          uint8_t *rxbuf,
                          size_t rxbytes);

/**
 * @brief Perform I2C master receive only
 *
 * Receives data from an I2C device without transmitting first.
 *
 * @param i2cp Pointer to I2C driver instance
 * @param addr 7-bit I2C device address
 * @param rxbuf Pointer to receive buffer
 * @param rxbytes Number of bytes to receive
 * @return MSG_OK on success, error code on failure
 */
msg_t i2c_master_receive(I2CDriver *i2cp, i2caddr_t addr, uint8_t *rxbuf, size_t rxbytes);

/**
 * @brief Handle and report I2C communication errors
 *
 * Analyzes I2C driver error flags and prints a human-readable
 * error message. This provides centralized error handling for all I2C
 * operations.
 *
 * @param i2c_driver Pointer to I2C driver instance
 * @param context Context string for error reporting (e.g., "read_reg",
 * "write_reg")
 */
void i2c_handle_error(I2CDriver *i2c_driver, const char *context);

/**
 * @brief Read one or more registers from I2C device
 *
 * Convenience function for register-based I2C devices. Transmits the
 * register address and receives the specified number of bytes.
 *
 * @param bus Pointer to I2C bus configuration
 * @param reg Register address to read from
 * @param buf Pointer to receive buffer
 * @param len Number of bytes to read
 * @return 0 on success, negative error code on failure
 */
int i2c_bus_read_reg(i2c_bus_t *bus, uint8_t reg, uint8_t *buf, size_t len);

/**
 * @brief Write one or more registers to I2C device
 *
 * Convenience function for register-based I2C devices. Transmits the
 * register address followed by the specified data bytes.
 *
 * @param bus Pointer to I2C bus configuration
 * @param reg Register address to write to
 * @param buf Pointer to data buffer
 * @param len Number of bytes to write
 * @return 0 on success, negative error code on failure
 */
int i2c_bus_write_reg(i2c_bus_t *bus, uint8_t reg, const uint8_t *buf, size_t len);
