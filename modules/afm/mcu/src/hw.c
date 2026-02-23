/**
 * some hardware initialization stuff
 */
#include <stdint.h>

#include "hw.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"

#include "sysinfo_regs.h"

#include "GOWIN_M1.h"


#define MFG_ID_MAX_LEN 9


// HW access
volatile struct sysinfo_regs *sysinfo = (struct sysinfo_regs *)APB_M1;
volatile struct gpio_regs *gpio       = (struct gpio_regs *)(APB_M1 + 0x20);


void sysinfo_get_mfg(const volatile struct sysinfo_regs *sysinfo,
                     char *buffer,
                     size_t buffer_size) {
  if (buffer == NULL || buffer_size < MFG_ID_MAX_LEN || sysinfo == NULL) {
    return;
  }

  uint32_t msb_val = sysinfo->mfg_code_A;
  uint32_t lsb_val = sysinfo->mfg_code_B;

  buffer[0] = (char)((msb_val >> 24) & 0xFF);
  buffer[1] = (char)((msb_val >> 16) & 0xFF);
  buffer[2] = (char)((msb_val >> 8) & 0xFF);
  buffer[3] = (char)((msb_val >> 0) & 0xFF);
  buffer[4] = (char)((lsb_val >> 24) & 0xFF);
  buffer[5] = (char)((lsb_val >> 16) & 0xFF);
  buffer[6] = (char)((lsb_val >> 8) & 0xFF);
  buffer[7] = (char)((lsb_val >> 0) & 0xFF);
  buffer[8] = '\0';
}


void hw_init(void) {
  // initialize the system, including the debug uart, gpio, and delay timer.
  SystemInit();

  debug_init();
  gpio_init();
  delay_init();

  dbg_printf("\r\n");

  dbg_printf("SystemCoreClock: %d mHz\r\n", SystemCoreClock / 1000000);

  char mfg_id_buffer[MFG_ID_MAX_LEN];
  sysinfo_get_mfg(sysinfo, mfg_id_buffer, sizeof(mfg_id_buffer));

  dbg_printf("magic: 0x%X\r\n", sysinfo->magic);
  dbg_printf("mfg_id: %s\r\n", mfg_id_buffer);
  dbg_printf("dev version: 0x%08X\r\n", sysinfo->version);
  dbg_printf("dev version: v%d.%d.%d\r\n",
             (sysinfo->version >> SYSINFO_REGS_VERSION_MAJOR_SHIFT) & 0xFF,
             (sysinfo->version >> SYSINFO_REGS_VERSION_MINOR_SHIFT) & 0xFF,
             (sysinfo->version >> SYSINFO_REGS_VERSION_PATCH_SHIFT) & 0xFF);
  dbg_printf("cheby version: v%d.%d.%d\r\n",
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MAJOR_SHIFT) & 0xFF,
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_MINOR_SHIFT) & 0xFF,
             (sysinfo->cheby_version >> SYSINFO_REGS_CHEBY_VERSION_PATCH_SHIFT) & 0xFF);
  //dbg_printf("gpio stat: 0x%08X\r\n", gpio->stat);
}
