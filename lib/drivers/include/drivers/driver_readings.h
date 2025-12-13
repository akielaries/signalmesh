#pragma once

#include <stdint.h>

// Forward declarations to break circular dependency
typedef struct device device_t;
typedef device_t *device_id_t;


/**
 * @brief Enum for common reading value types.
 * Add more as needed.
 */
typedef enum {
  READING_VALUE_TYPE_UNKNOWN = 0,
  READING_VALUE_TYPE_UINT32,
  READING_VALUE_TYPE_FLOAT,
  // Add other types like INT32, BOOL, etc.
} reading_value_type_t;

/**
 * @brief Structure to hold a single driver reading.
 * Uses a union to support different data types.
 */
typedef struct {
  reading_value_type_t type;
  union {
    uint32_t u32_val;
    float float_val;
    // Add other members for different types
  } value;
} driver_reading_t;

/**
 * @brief Structure to describe a single channel in the driver readings
 * directory.
 */
typedef struct {
  const char *name;

  reading_value_type_t type;
} driver_reading_channel_t;

/**
 * @brief Structure for the driver readings directory.
 * Contains the number of readings and an array of channel descriptions.
 */
typedef struct {
  uint32_t num_readings;
  const driver_reading_channel_t *channels;
} driver_readings_directory_t;
