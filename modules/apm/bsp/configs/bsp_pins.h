#pragma once

#include "hal.h"

// A simple structure to hold a port and a pin/pad number
typedef struct {
    ioportid_t port;
    uint16_t   pad;
} bsp_pin_t;
