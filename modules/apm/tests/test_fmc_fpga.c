// FMC multiplexed-mode register bridge to the FPGA (memory-mapped HW/SW iface).
//
// 16-bit version. there is no ChibiOS FMC NOR/PSRAM driver, so we set the FMC
// registers directly:
//   - enable the FMC clock (AHB3)
//   - bank 1 (NE1): MUXEN=1 (address on AD bus, latched by NADV), MWID=16,
//     MTYP=PSRAM, async, WREN, very slow timing for the breadboard PoC
//   - MPU marks the window non-cacheable (Device) so reads/writes hit the FPGA
//
// FPGA register map (16-bit, word-addressed; STM32 byte offset = word * 2):
//   0x00 ID       RO  0xACE1
//   0x02 SCRATCH  RW
//   0x04 LED_CTRL RW  low 5 bits -> FPGA LED[4:0]
//
// wiring: STM32 AD0..15 / NL / NOE / NWE / NE1 / NWAIT jumpered to the FPGA,
// common ground.

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#define FMC_FPGA_BASE 0x60000000UL
#define REG_ID        0x00U
#define REG_SCRATCH   0x02U
#define REG_LED       0x04U
#define FPGA_ID       0xACE1U

// the FMC window is non-cacheable (Device) via the MPU region in fmc_mpu_init,
// so plain volatile access goes straight to the FPGA - no cache maintenance.
static inline uint16_t fpga_rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}

static inline void fpga_wr(uint32_t off, uint16_t val) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = val;
}

// configure the FMC pins as alternate function (AF12, except NE1/NWAIT = AF9)
static void fmc_gpio_init(void) {
  const iomode_t af12 = PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST;
  const iomode_t af9  = PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_HIGHEST;

  // AD bus (16-bit: D0..D15)
  palSetPadMode(GPIOD, 14, af12);  // AD0
  palSetPadMode(GPIOD, 15, af12);  // AD1
  palSetPadMode(GPIOD, 0,  af12);  // AD2
  palSetPadMode(GPIOD, 1,  af12);  // AD3
  palSetPadMode(GPIOE, 7,  af12);  // AD4
  palSetPadMode(GPIOE, 8,  af12);  // AD5
  palSetPadMode(GPIOE, 9,  af12);  // AD6
  palSetPadMode(GPIOE, 10, af12);  // AD7
  palSetPadMode(GPIOE, 11, af12);  // AD8
  palSetPadMode(GPIOE, 12, af12);  // AD9
  palSetPadMode(GPIOE, 13, af12);  // AD10
  palSetPadMode(GPIOE, 14, af12);  // AD11
  palSetPadMode(GPIOE, 15, af12);  // AD12
  palSetPadMode(GPIOD, 8,  af12);  // AD13
  palSetPadMode(GPIOD, 9,  af12);  // AD14
  palSetPadMode(GPIOD, 10, af12);  // AD15

  // control
  palSetPadMode(GPIOB, 7,  af12);  // NL (NADV)
  palSetPadMode(GPIOD, 4,  af12);  // NOE
  palSetPadMode(GPIOD, 5,  af12);  // NWE
  palSetPadMode(GPIOC, 7,  af9);   // NE1
  palSetPadMode(GPIOC, 6,  af9);   // NWAIT
}

// bank 1 (NE1): muxed PSRAM, 16-bit, async, write enabled, very slow timing
static void fmc_ctrl_init(void) {
  rccEnableAHB3(RCC_AHB3ENR_FMCEN, true);

  // timing (FMC clock cycles): ADDSET=15, ADDHLD=15, DATAST=255, BUSTURN=15
  FMC_Bank1_R->BTCR[1] = (15U  << FMC_BTRx_ADDSET_Pos)  |
                         (15U  << FMC_BTRx_ADDHLD_Pos)  |
                         (255U << FMC_BTRx_DATAST_Pos)  |
                         (15U  << FMC_BTRx_BUSTURN_Pos);

  // control: global FMC enable + bank enable + muxed + PSRAM + 16-bit + write
  FMC_Bank1_R->BTCR[0] = FMC_BCR1_FMCEN  |
                         FMC_BCRx_MBKEN  |
                         FMC_BCRx_MUXEN  |
                         FMC_BCRx_MTYP_0 |   // PSRAM
                         FMC_BCRx_MWID_0 |   // 16-bit
                         FMC_BCRx_WREN;
  __DSB();
}

// mark the FMC window non-cacheable (Device) so register access bypasses cache.
// build RASR by hand - the ARM_MPU_RASR() macro dropped the enable+size fields
// on this CMSIS, leaving the region disabled.
static void fmc_mpu_init(void) {
  ARM_MPU_Disable();
  MPU->RNR  = 7U;
  MPU->RBAR = FMC_FPGA_BASE;                 // base; VALID=0 -> use RNR
  MPU->RASR = MPU_RASR_ENABLE_Msk        |
              (19U << MPU_RASR_SIZE_Pos) |   // 2^(19+1) = 1 MB
              (1U  << MPU_RASR_XN_Pos)   |   // no execute
              (3U  << MPU_RASR_AP_Pos)   |   // RW full access
              (1U  << MPU_RASR_B_Pos)    |   // TEX=0 C=0 B=1 S=1 -> Device
              (1U  << MPU_RASR_S_Pos);
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
  SCB_CleanInvalidateDCache();               // drop lines cached while it was normal
  __DSB();
  __ISB();

  MPU->RNR = 7U;
  bsp_printf("MPU CTRL=0x%08lX rgn7 RBAR=0x%08lX RASR=0x%08lX SCB_CCR=0x%08lX\n",
             (unsigned long)MPU->CTRL, (unsigned long)MPU->RBAR,
             (unsigned long)MPU->RASR, (unsigned long)SCB->CCR);
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fmc_fpga: FMC muxed register bridge (16-bit) ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();
  bsp_printf("FMC up (muxed, 16-bit, slow). reading FPGA...\n");

  // 1. presence handshake
  uint16_t id = fpga_rd(REG_ID);
  bsp_printf("ID = 0x%04X (expect 0x%04X) %s\n",
             (unsigned)id, (unsigned)FPGA_ID,
             (id == FPGA_ID) ? "OK" : "MISMATCH");

  // 2. scratch round-trip
  fpga_wr(REG_SCRATCH, 0x1234U);
  uint16_t sc = fpga_rd(REG_SCRATCH);
  bsp_printf("SCRATCH wrote 0x1234 read 0x%04X %s\n",
             (unsigned)sc, (sc == 0x1234U) ? "OK" : "MISMATCH");

  // 3. chase LED0..LED4 one at a time over FMC (LED5 is the RTL boot blink)
  bsp_printf("chasing FPGA LEDs via LED_CTRL...\n");
  uint16_t pat = 0x01U;
  while (true) {
    fpga_wr(REG_LED, pat);
    pat <<= 1;
    if (pat > 0x10U) {
      pat = 0x01U;
    }
    chThdSleepMilliseconds(250);
  }
}
