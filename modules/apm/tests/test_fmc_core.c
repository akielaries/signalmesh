// FMC core-register test: reads the cheby-defined `core` registers from the FPGA
// over FMC. confirms the link (magic), identifies the board (fpga_id), and
// round-trips scratch. uses the generated core_regs.h offsets - one source of
// truth shared with the FPGA RTL (cheby/core_regs.yaml).
//
//   MAGIC   = 0xACE1               link / gateware check
//   FPGA_ID = 0x2018 GW2AR-18      (Tang Nano 20K)  /  0x5025 GW5A-25 (Primer 25K)
//   SCRATCH = RW                   round-trip + walks LED[4:0] on the board

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "cheby/core_regs.h"


#define FMC_FPGA_BASE 0x60000000UL


static inline uint16_t fpga_rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}
static inline void fpga_wr(uint32_t off, uint16_t val) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = val;
}

// ---- FMC bring-up (16-bit muxed async, same as the earlier FMC tests) ------
static void fmc_gpio_init(void) {
  const iomode_t af12 = PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_LOWEST;
  const iomode_t af9  = PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST;
  palSetPadMode(GPIOD, 14, af12); palSetPadMode(GPIOD, 15, af12);
  palSetPadMode(GPIOD, 0,  af12); palSetPadMode(GPIOD, 1,  af12);
  palSetPadMode(GPIOE, 7,  af12); palSetPadMode(GPIOE, 8,  af12);
  palSetPadMode(GPIOE, 9,  af12); palSetPadMode(GPIOE, 10, af12);
  palSetPadMode(GPIOE, 11, af12); palSetPadMode(GPIOE, 12, af12);
  palSetPadMode(GPIOE, 13, af12); palSetPadMode(GPIOE, 14, af12);
  palSetPadMode(GPIOE, 15, af12); palSetPadMode(GPIOD, 8,  af12);
  palSetPadMode(GPIOD, 9,  af12); palSetPadMode(GPIOD, 10, af12);
  palSetPadMode(GPIOB, 7,  af12); palSetPadMode(GPIOD, 4,  af12);
  palSetPadMode(GPIOD, 5,  af12); palSetPadMode(GPIOC, 7,  af9);
  palSetPadMode(GPIOC, 6,  af9);
}

static void fmc_ctrl_init(void) {
  rccEnableAHB3(RCC_AHB3ENR_FMCEN, true);
  FMC_Bank1_R->BTCR[1] = (15U << FMC_BTRx_ADDSET_Pos) | (15U << FMC_BTRx_ADDHLD_Pos) |
                         (255U << FMC_BTRx_DATAST_Pos) | (15U << FMC_BTRx_BUSTURN_Pos);
  FMC_Bank1_R->BTCR[0] = FMC_BCR1_FMCEN | FMC_BCRx_MBKEN | FMC_BCRx_MUXEN |
                         FMC_BCRx_MTYP_0 | FMC_BCRx_MWID_0 | FMC_BCRx_WREN;
  __DSB();
}

static void fmc_mpu_init(void) {
  ARM_MPU_Disable();
  MPU->RNR  = 7U;
  MPU->RBAR = FMC_FPGA_BASE;
  MPU->RASR = MPU_RASR_ENABLE_Msk | (19U << MPU_RASR_SIZE_Pos) |
              (1U << MPU_RASR_XN_Pos) | (3U << MPU_RASR_AP_Pos) |
              (1U << MPU_RASR_B_Pos) | (1U << MPU_RASR_S_Pos);
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
  SCB_CleanInvalidateDCache();
  __DSB();
  __ISB();
}

int main(void) {
  bsp_init();
  bsp_printf("testing cheby core regs over FMC conn\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();

  uint16_t magic = fpga_rd(CORE_MAGIC);
  bsp_printf("MAGIC   = 0x%04X %s\n", (unsigned)magic,
             (magic == 0xACE1U) ? "OK" : "MISMATCH");

  uint16_t fid = fpga_rd(CORE_FPGA_ID);
  const char *board = (fid == 0x2018U) ? "GW2AR-18 (Tang Nano 20K)" :
                      (fid == 0x5025U) ? "GW5A-25 (Tang Primer 25K)" : "unknown";
  bsp_printf("FPGA_ID = 0x%04X (%s)\n", (unsigned)fid, board);

  bsp_printf("VERSION = 0x%04X\n", (unsigned)fpga_rd(CORE_VERSION));

  // scratch round-trip
  fpga_wr(CORE_SCRATCH, 0x1234U);
  uint16_t sc = fpga_rd(CORE_SCRATCH);
  bsp_printf("SCRATCH wrote 0x1234, read 0x%04X %s\n", (unsigned)sc,
             (sc == 0x1234U) ? "OK" : "FAIL");

  // walk a single bit through scratch - LED[4:0] on the board mirrors it
  uint16_t pat = 0x01U;
  while (true) {
    fpga_wr(CORE_SCRATCH, pat);
    bsp_printf("scratch=0x%04X\n", (unsigned)pat);
    pat <<= 1;
    if (pat > 0x10U) {
      pat = 0x01U;
    }
    chThdSleepMilliseconds(400);
  }
}
