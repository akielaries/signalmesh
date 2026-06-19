// FMC multiplexed-mode register bridge to the FPGA (memory-mapped HW/SW iface).
//
// the FPGA is an FMC muxed-PSRAM slave at 0x60000000. there is no ChibiOS FMC
// NOR/PSRAM driver, so we set the FMC registers directly:
//   - enable the FMC clock (AHB3)
//   - bank 1 (NE1): MUXEN=1 (address on the AD bus, latched by NADV), MWID=16,
//     MTYP=PSRAM, async, WREN, very slow timing for the breadboard PoC
//   - MPU marks the window non-cacheable (Device) so reads/writes hit the FPGA,
//     not the d-cache
//
// FPGA register map (16-bit; STM32 byte offset = fpga word index * 2):
//   0x00 ID       RO  0xACE1
//   0x02 SCRATCH  RW
//   0x04 LED_CTRL RW  low 5 bits -> FPGA LED[4:0]
//
// wiring: STM32 AD0..15 / NL / NOE / NWE / NE1 / NWAIT jumpered to the FPGA,
// common ground. run slow (handled by the timing below).

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

static inline uint16_t fpga_rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}

static inline void fpga_wr(uint32_t off, uint16_t val) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = val;
}

// configure all FMC pins as alternate function (AF12, except NE1/NWAIT = AF9)
static void fmc_gpio_init(void) {
  const iomode_t af12 = PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST;
  const iomode_t af9  = PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_HIGHEST;

  // AD bus (multiplexed address/data)
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

  // timing (HCLK cycles): ADDSET=15, ADDHLD=15, DATAST=255, BUSTURN=15 -> slow
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

// mark the FMC window non-cacheable (Device) so register access bypasses cache
static void fmc_mpu_init(void) {
  ARM_MPU_Disable();
  // region 7, base 0x60000000, 1 MB, XN, RW, Device (TEX=0 S=1 C=0 B=1)
  ARM_MPU_SetRegionEx(7U, FMC_FPGA_BASE,
      ARM_MPU_RASR(1U, ARM_MPU_AP_FULL, 0U, 1U, 0U, 1U, 0U,
                   ARM_MPU_REGION_SIZE_1MB));
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
  __DSB();
  __ISB();
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fmc_fpga: FMC muxed register bridge ---\r\n");

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

  // 3. drive the FPGA LEDs over FMC - visible end-to-end proof
  bsp_printf("walking FPGA LEDs via LED_CTRL...\n");
  uint16_t pat = 0x01U;
  while (true) {
    fpga_wr(REG_LED, pat);
    uint16_t rb = fpga_rd(REG_LED) & 0x1FU;
    bsp_printf("LED_CTRL=0x%02X readback=0x%02X %s\n",
               (unsigned)(pat & 0x1FU), (unsigned)rb,
               (rb == (pat & 0x1FU)) ? "" : "MISMATCH");
    pat <<= 1;
    if (pat > 0x10U) {
      pat = 0x01U;                 // wrap across the 5 LEDs
    }
    chThdSleepMilliseconds(300);
  }
}
