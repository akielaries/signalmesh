#pragma once

#include <stdint.h>

/**
 * @brief Enum for common reading value types.
 */
typedef enum {
  READING_VALUE_TYPE_UNKNOWN = 0,
  READING_VALUE_TYPE_UINT32,
  READING_VALUE_TYPE_INT32,
  READING_VALUE_TYPE_FLOAT,
} reading_value_type_t;

typedef enum {
    READING_CHANNEL_TYPE_TEMPERATURE_C,
    READING_CHANNEL_TYPE_TEMPERATURE_F,
    READING_CHANNEL_TYPE_PRESSURE_PA,
    READING_CHANNEL_TYPE_PRESSURE_PSI,
    READING_CHANNEL_TYPE_PRESSURE_INHG,
    READING_CHANNEL_TYPE_HUMIDITY,
    READING_CHANNEL_TYPE_VOLTAGE,
    READING_CHANNEL_TYPE_CURRENT,
    READING_CHANNEL_TYPE_POWER,
    READING_CHANNEL_TYPE_POSITION_US,
    READING_CHANNEL_TYPE_MAX, // Keep this last
} reading_channel_type_t;

/**
 * @brief Structure to hold a single driver reading.
 */
typedef struct {
  reading_value_type_t type;
  union {
    uint32_t u32_val;
    int32_t i32_val;
    float float_val;
  } value;
} driver_reading_t;

typedef struct {
  reading_channel_type_t channel_type;
  const char *name;
  const char *unit;
  reading_value_type_t type;
} driver_reading_channel_t;

/**
 * @brief Structure for the driver readings directory.
 */
typedef struct {
  uint32_t num_readings;
  const driver_reading_channel_t *channels;
} driver_readings_directory_t;
