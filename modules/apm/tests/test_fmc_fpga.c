// FMC multiplexed-mode register bridge to the FPGA (memory-mapped HW/SW iface).
//
// 8-bit version: half the wiring of the 16-bit bus, and it drops PD8-PD15
// (including the cursed AD14/PD9). there is no ChibiOS FMC NOR/PSRAM driver, so
// we set the FMC registers directly:
//   - enable the FMC clock (AHB3)
//   - bank 1 (NE1): MUXEN=1 (address on AD bus, latched by NADV), MWID=8,
//     MTYP=PSRAM, async, WREN, very slow timing for the breadboard PoC
//   - MPU marks the window non-cacheable (Device) so reads/writes hit the FPGA
//
// FPGA register map (8-bit, byte-addressed - no shift in 8-bit mode):
//   0x00 ID       RO  0xA5
//   0x01 SCRATCH  RW
//   0x02 LED_CTRL RW  low 5 bits -> FPGA LED[4:0]
//
// wiring: STM32 AD0..7 / NL / NOE / NWE / NE1 / NWAIT jumpered to the FPGA,
// common ground.

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#define FMC_FPGA_BASE 0x60000000UL
#define REG_ID        0x00U
#define REG_SCRATCH   0x01U
#define REG_LED       0x02U
#define FPGA_ID       0xA5U

static inline uint8_t fpga_rd(uint32_t off) {
  return *(volatile uint8_t *)(FMC_FPGA_BASE + off);
}

static inline void fpga_wr(uint32_t off, uint8_t val) {
  *(volatile uint8_t *)(FMC_FPGA_BASE + off) = val;
}

// configure the FMC pins as alternate function (AF12, except NE1/NWAIT = AF9)
static void fmc_gpio_init(void) {
  const iomode_t af12 = PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST;
  const iomode_t af9  = PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_HIGHEST;

  // AD bus (8-bit: D0..D7)
  palSetPadMode(GPIOD, 14, af12);  // AD0
  palSetPadMode(GPIOD, 15, af12);  // AD1
  palSetPadMode(GPIOD, 0,  af12);  // AD2
  palSetPadMode(GPIOD, 1,  af12);  // AD3
  palSetPadMode(GPIOE, 7,  af12);  // AD4
  palSetPadMode(GPIOE, 8,  af12);  // AD5
  palSetPadMode(GPIOE, 9,  af12);  // AD6
  palSetPadMode(GPIOE, 10, af12);  // AD7

  // control
  palSetPadMode(GPIOB, 7,  af12);  // NL (NADV)
  palSetPadMode(GPIOD, 4,  af12);  // NOE
  palSetPadMode(GPIOD, 5,  af12);  // NWE
  palSetPadMode(GPIOC, 7,  af9);   // NE1
  palSetPadMode(GPIOC, 6,  af9);   // NWAIT
}

// bank 1 (NE1): muxed PSRAM, 8-bit, async, write enabled, very slow timing
static void fmc_ctrl_init(void) {
  rccEnableAHB3(RCC_AHB3ENR_FMCEN, true);

  // timing (FMC clock cycles): ADDSET=15, ADDHLD=15, DATAST=255, BUSTURN=15
  FMC_Bank1_R->BTCR[1] = (15U  << FMC_BTRx_ADDSET_Pos)  |
                         (15U  << FMC_BTRx_ADDHLD_Pos)  |
                         (255U << FMC_BTRx_DATAST_Pos)  |
                         (15U  << FMC_BTRx_BUSTURN_Pos);

  // control: global FMC enable + bank enable + muxed + PSRAM + 8-bit + write.
  // MWID=00 (8-bit) so no FMC_BCRx_MWID_0 bit here.
  FMC_Bank1_R->BTCR[0] = FMC_BCR1_FMCEN  |
                         FMC_BCRx_MBKEN  |
                         FMC_BCRx_MUXEN  |
                         FMC_BCRx_MTYP_0 |   // PSRAM
                         FMC_BCRx_WREN;
  __DSB();
}

// mark the FMC window non-cacheable (Device) so register access bypasses cache
static void fmc_mpu_init(void) {
  ARM_MPU_Disable();
  ARM_MPU_SetRegionEx(7U, FMC_FPGA_BASE,
      ARM_MPU_RASR(1U, ARM_MPU_AP_FULL, 0U, 1U, 0U, 1U, 0U,
                   ARM_MPU_REGION_SIZE_1MB));
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
  __DSB();
  __ISB();
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fmc_fpga: FMC muxed register bridge (8-bit) ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();
  bsp_printf("FMC up (muxed, 8-bit, slow). reading FPGA...\n");

  // 1. presence handshake
  uint8_t id = fpga_rd(REG_ID);
  bsp_printf("ID = 0x%02X (expect 0x%02X) %s\n",
             (unsigned)id, (unsigned)FPGA_ID,
             (id == FPGA_ID) ? "OK" : "MISMATCH");

  // 2. scratch round-trip
  fpga_wr(REG_SCRATCH, 0x5AU);
  uint8_t sc = fpga_rd(REG_SCRATCH);
  bsp_printf("SCRATCH wrote 0x5A read 0x%02X %s\n",
             (unsigned)sc, (sc == 0x5AU) ? "OK" : "MISMATCH");

  // 3. drive the FPGA LEDs over FMC - visible end-to-end proof
  bsp_printf("walking FPGA LEDs via LED_CTRL...\n");
  uint8_t pat = 0x01U;
  while (true) {
    fpga_wr(REG_LED, pat);
    uint8_t rb = fpga_rd(REG_LED) & 0x1FU;
    bsp_printf("LED_CTRL=0x%02X readback=0x%02X %s\n",
               (unsigned)(pat & 0x1FU), (unsigned)rb,
               (rb == (pat & 0x1FU)) ? "" : "MISMATCH");
    pat <<= 1;
    if (pat > 0x10U) {
      pat = 0x01U;
    }
    chThdSleepMilliseconds(2000);
  }
}
