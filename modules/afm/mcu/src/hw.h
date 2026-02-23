#pragma once

#include "sysinfo_regs.h"

extern volatile struct sysinfo_regs *sysinfo;

void hw_init(void);

