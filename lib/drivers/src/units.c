#include <stddef.h>

#include "drivers/units.h"


typedef struct {
  reading_channel_type_t channel_type;
  const char *unit_string;
} unit_mapping_t;


static const unit_mapping_t unit_mappings[] = {
  {READING_CHANNEL_TYPE_TEMPERATURE_C, "C"},
  {READING_CHANNEL_TYPE_TEMPERATURE_F, "F"},
  {READING_CHANNEL_TYPE_PRESSURE_PA, "Pa"},
  {READING_CHANNEL_TYPE_PRESSURE_PSI, "PSI"},
  {READING_CHANNEL_TYPE_PRESSURE_INHG, "inHg"},
  {READING_CHANNEL_TYPE_HUMIDITY, "%RH"},
  {READING_CHANNEL_TYPE_VOLTAGE, "V"},
  {READING_CHANNEL_TYPE_CURRENT, "mA"},
  {READING_CHANNEL_TYPE_POWER, "mW"},
};

const char *get_unit_for_reading(reading_channel_type_t type) {
  for (size_t i = 0; i < sizeof(unit_mappings) / sizeof(unit_mappings[0]); ++i) {
    if (unit_mappings[i].channel_type == type) {
      return unit_mappings[i].unit_string;
    }
  }
  return "";
}
