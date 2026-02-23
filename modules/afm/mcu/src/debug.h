#pragma once

#include <stdarg.h>
#include "GOWIN_M1_uart.h"

void debug_init(void);
void dbg_printf(const char* format, ...);

