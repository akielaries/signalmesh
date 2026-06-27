// FMC audio test: the FPGA generates a waveform (one period, 256 x 12-bit) in
// its FMC register space; the STM32 reads it over FMC into a buffer and plays
// it out DAC1 CH1 (PA4) via TIM6-triggered circular DMA.
//
// chain:  FPGA wavetable -> FMC AUDIO region -> SRAM buffer -> DAC1/PA4
// tone:   48 kHz / 256 samples = 187.5 Hz; WAVE_SEL cycles sine/saw/square/tri
//
// FPGA slave map: WAVE_SEL @ word 3 (0x06), AUDIO @ words 0x100..0x1FF (0x200).

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#define FMC_FPGA_BASE 0x60000000UL
#define REG_ID        0x00U
#define REG_WAVE_SEL  0x06U          // word 3
#define AUDIO_OFF     0x200U         // word 0x100 -> byte 0x200
#define FPGA_ID       0xACE1U

#define N_SAMPLES     256U
#define SAMPLE_RATE   48000U         // -> 187.5 Hz tone

static inline uint16_t fpga_rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}
static inline void fpga_wr(uint32_t off, uint16_t val) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = val;
}

// ---- FMC bring-up (same as test_fmc_fpga.c) -------------------------------
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
                         (15U << FMC_BTRx_DATAST_Pos) | (15U << FMC_BTRx_BUSTURN_Pos);
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

// ---- DAC + timer ----------------------------------------------------------
// DMA-aligned so we can clean the cache after filling it from FMC.
CC_ALIGN_DATA(32) static dacsample_t audio_buf[N_SAMPLES];

static const DACConfig dac_cfg = {
  .init     = 2048U,                 // mid-scale at startup
  .datamode = DAC_DHRM_12BIT_RIGHT,
  .cr       = 0U
};

static const DACConversionGroup dac_grpcfg = {
  .num_channels = 1U,
  .end_cb       = NULL,
  .error_cb     = NULL,
  // TIM6 TRGO. on H7 this is TSEL=5 (F4/L1 had TIM6 at TSEL=0 - H7 remapped it;
  // TSEL=0 on H7 is software trigger, which left the DAC parked at init)
  .trigger      = DAC_TRG(5)
};

static const GPTConfig gpt_cfg = {
  .frequency = 1200000U,             // 1.2 MHz timebase
  .callback  = NULL,
  .cr2       = TIM_CR2_MMS_1,        // MMS=010: TRGO on update event
  .dier      = 0U
};

// read one period of the current waveform from the FPGA over FMC
static void load_waveform(void) {
  for (uint32_t i = 0; i < N_SAMPLES; i++) {
    audio_buf[i] = (dacsample_t)(fpga_rd(AUDIO_OFF + i * 2U) & 0x0FFFU);
  }
  cacheBufferFlush(audio_buf, sizeof(audio_buf));   // so the DAC DMA sees it
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- fmc_audio_test: FPGA wavetable -> FMC -> DAC1/PA4 ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();

  uint16_t id = fpga_rd(REG_ID);
  bsp_printf("FPGA ID = 0x%04X %s\n", (unsigned)id,
             (id == FPGA_ID) ? "OK" : "MISMATCH");

  // PA4 = DAC1_OUT1, analog
  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);

  // start with sine, load it, then start the DAC stream
  fpga_wr(REG_WAVE_SEL, 0U);
  load_waveform();

  dacStart(&DACD1, &dac_cfg);
  gptStart(&GPTD6, &gpt_cfg);
  dacStartConversion(&DACD1, &dac_grpcfg, audio_buf, N_SAMPLES);
  gptStartContinuous(&GPTD6, 1200000U / SAMPLE_RATE);   // 25 -> 48 kHz

  bsp_printf("playing %luHz tone, cycling waveforms...\n",
             (unsigned long)(SAMPLE_RATE / N_SAMPLES));

  // cycle sine/saw/square/triangle every 2 s - timbre changes audibly
  const char *names[4] = {"sine", "saw", "square", "triangle"};
  uint8_t sel = 0;
  while (true) {
    fpga_wr(REG_WAVE_SEL, sel);
    load_waveform();                 // refill in place; DAC DMA picks it up
    bsp_printf("WAVE_SEL=%u (%s)\n", (unsigned)sel, names[sel]);
    sel = (sel + 1U) & 3U;
    chThdSleepMilliseconds(2000);
  }
}
