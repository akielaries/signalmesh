#include <stddef.h>
#include "drivers/units.h"

static const reading_channel_info_t channel_info_table[READING_CHANNEL_TYPE_MAX] = {
    [READING_CHANNEL_TYPE_TEMPERATURE_C] = { "temperature", "C", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_TEMPERATURE_F] = { "temperature", "F", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_PRESSURE_PA] = { "pressure", "Pa", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_PRESSURE_PSI] = { "pressure", "PSI", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_PRESSURE_INHG] = { "pressure", "inHg", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_HUMIDITY] = { "humidity", "%RH", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_VOLTAGE] = { "voltage", "V", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_CURRENT] = { "current", "mA", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_POWER] = { "power", "mW", READING_VALUE_TYPE_FLOAT },
    [READING_CHANNEL_TYPE_POSITION_US] = { "position", "us", READING_VALUE_TYPE_UINT32 },
};

const reading_channel_info_t* get_channel_info(reading_channel_type_t type) {
    if (type < READING_CHANNEL_TYPE_MAX) {
        return &channel_info_table[type];
    }
    return NULL;
}

