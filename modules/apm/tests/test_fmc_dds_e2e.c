// end-to-end DDS test over FMC, now with audio out the DAC.
//   STM32 writes a voice -> FPGA DDS synthesizes -> STM32 streams the mixed
//   `sample` register to DAC1/PA4 at Fs.
//
// FMC byte map (see acm_top.v): core @ 0x00, audio @ 0x80.
//
// streaming model: the FPGA produces one mixed sample per its own Fs tick into
// the `sample` register (no FIFO). a GPT (TIM6) callback at 48 kHz reads that
// register, converts signed-16 -> 12-bit unsigned, and writes the DAC. the two
// 48 kHz clocks (STM32 GPT vs FPGA TICK_DIV) aren't locked, so expect minor
// duplicate/skip jitter - fine for a smoke test; a sample FIFO + flow control
// is the clean fix later.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "cheby/core_regs.h"          // CORE_MAGIC / CORE_FPGA_ID (sanity)
#include "cheby/audio_regs.h"         // AUDIO_* offsets (relative to the audio block)

#define FMC_FPGA_BASE 0x60000000UL
#define AUDIO_BASE    0x80UL          // audio block sits at FMC byte 0x80 in acm_top

static inline uint16_t rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}
static inline void wr(uint32_t off, uint16_t v) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = v;
}

// audio register byte addresses (audio block base + cheby offset)
#define A_SAMPLE   (AUDIO_BASE + AUDIO_SAMPLE)
#define A_V0_FREQ  (AUDIO_BASE + AUDIO_VOICE + AUDIO_VOICE_FREQ)   // 32-bit
#define A_V0_CTRL  (AUDIO_BASE + AUDIO_VOICE + AUDIO_VOICE_CTRL)   // gate/wave/level

// cheby lays a 32-bit register big-endian over the 16-bit bus:
//   low address = high half [31:16], high address = low half [15:0]
static inline void wr_freq32(uint32_t base, uint32_t f) {
  wr(base + 0, (uint16_t)(f >> 16));
  wr(base + 2, (uint16_t)(f & 0xFFFFU));
}

// voice ctrl: gate(b0) | wave(b3:1, 0=sine 1=saw 2=square 3=tri) | level(b15:8)
static inline uint16_t voice_ctrl(uint8_t gate, uint8_t wave, uint8_t level) {
  return (uint16_t)((gate & 1U) | ((wave & 7U) << 1) | ((uint16_t)level << 8));
}

// ---- FMC bring-up (identical to test_fmc_core.c) ------------------------------
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

// ---- DAC streaming -----------------------------------------------------------
#define SAMPLE_RATE   48000U
#define GPT_HZ        1200000U        // GPT timebase; 1.2 MHz / 48 kHz = 25

// software-mode DAC: writes to DHR update the output directly (no trigger/DMA),
// so the GPT callback can push one live sample per tick.
static const DACConfig dac_cfg = {
  .init     = 2048U,                  // mid-scale at startup
  .datamode = DAC_DHRM_12BIT_RIGHT,
  .cr       = 0U
};

// signed-16 DDS sample -> 12-bit unsigned DAC code.
// a proper full-scale 8-voice mix would be (s >> 4) + 2048; a single gated voice
// only swings ~+/-4096 through the >>3 mix headroom, so we shift by 1 (8x louder)
// for an audible demo, clamped to the 12-bit range.
static inline uint16_t sample_to_dac(int16_t s) {
  int32_t v = ((int32_t)s >> 1) + 2048;
  if (v < 0)    { v = 0; }
  if (v > 4095) { v = 4095; }
  return (uint16_t)v;
}

// fired at Fs: pull the FPGA's latest mixed sample over FMC and drive the DAC.
static void dac_tick_cb(GPTDriver *gptp) {
  (void)gptp;
  int16_t s = (int16_t)rd(A_SAMPLE);
  DAC1->DHR12R1 = sample_to_dac(s);   // CH1 (PA4), 12-bit right-aligned
}

static const GPTConfig gpt_cfg = {
  .frequency = GPT_HZ,
  .callback  = dac_tick_cb,
  .cr2       = 0U,
  .dier      = 0U
};

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fmc_dds_e2e: STM32 -> FMC -> DDS -> DAC/PA4 ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();

  // sanity: core still reachable through acm_top at 0x00
  uint16_t magic = rd(CORE_MAGIC);
  bsp_printf("MAGIC   = 0x%04X %s\n", (unsigned)magic, (magic == 0xACE1U) ? "OK" : "FAIL");
  bsp_printf("FPGA_ID = 0x%04X\n", (unsigned)rd(CORE_FPGA_ID));

  // configure voice 0: ~375 Hz, full level, gated on
  // (osc freq = tuning_word * Fs / 2^32; 0x02000000 @ 48 kHz ~= 375 Hz)
  const uint32_t tuning = 0x02000000UL;
  wr_freq32(A_V0_FREQ, tuning);
  wr(A_V0_CTRL, voice_ctrl(1, 0, 0xFF));   // start on sine
  bsp_printf("voice0: freq=0x%08lX, gated, level 0xFF\n", (unsigned long)tuning);

  // PA4 = DAC1_OUT1, analog; start the DAC and the Fs streaming timer
  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
  dacStart(&DACD1, &dac_cfg);
  gptStart(&GPTD6, &gpt_cfg);
  gptStartContinuous(&GPTD6, GPT_HZ / SAMPLE_RATE);   // 25 -> 48 kHz

  bsp_printf("streaming to DAC; cycling waveforms every 2 s...\n");

  // cycle sine/saw/square/triangle so the timbre change is audible on PA4
  const char *names[4] = {"sine", "saw", "square", "triangle"};
  uint8_t wave = 0;
  while (true) {
    wr(A_V0_CTRL, voice_ctrl(1, wave, 0xFF));
    bsp_printf("wave = %u (%s), sample = %6d\n",
               (unsigned)wave, names[wave], (int)(int16_t)rd(A_SAMPLE));
    wave = (uint8_t)((wave + 1U) & 3U);
    chThdSleepMilliseconds(2000);
  }
}
