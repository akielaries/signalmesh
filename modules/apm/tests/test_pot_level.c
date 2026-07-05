// level proof: one always-on DDS voice at A440, its `level` register driven by the
// release pot (mux channel 2, read on COM_B / PC3). turn the pot -> the tone gets
// louder/quieter. proves the control path: pot -> ADC -> FMC voice.level -> FPGA
// DDS (does the wave*level multiply + mix per sample) -> dac reg -> DMA -> DAC.
// the envelope later just makes `level` time-varying instead of straight from the pot.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/adc.h"
#include "cheby/core_regs.h"
#include "cheby/audio_regs.h"

// ---- FMC + audio register access (from test_midi_synth.c) --------------------
#define FMC_FPGA_BASE 0x60000000UL
#define AUDIO_BASE    0x80UL

static inline uint16_t rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}
static inline void wr(uint32_t off, uint16_t v) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = v;
}

#define A_DAC           (AUDIO_BASE + AUDIO_DAC)
#define A_VOICE_FREQ(i) (AUDIO_BASE + AUDIO_VOICE + (i) * AUDIO_VOICE_SIZE + AUDIO_VOICE_FREQ)
#define A_VOICE_CTRL(i) (AUDIO_BASE + AUDIO_VOICE + (i) * AUDIO_VOICE_SIZE + AUDIO_VOICE_CTRL)

static inline void wr_freq32(uint32_t base, uint32_t f) {
  wr(base + 0, (uint16_t)(f >> 16));      // cheby big-endian: high half low addr
  wr(base + 2, (uint16_t)(f & 0xFFFFU));
}
static inline uint16_t voice_ctrl(uint8_t gate, uint8_t wave, uint8_t level) {
  return (uint16_t)((gate & 1U) | ((wave & 7U) << 1) | ((uint16_t)level << 8));
}

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
  __DSB(); __ISB();
}

// ---- DAC streaming (from test_midi_synth.c) ---------------------------------
#define SAMPLE_RATE 48000U
#define GPT_HZ      1200000U

static const DACConfig dac_cfg = {
  .init = 2048U, .datamode = DAC_DHRM_12BIT_RIGHT, .cr = 0U
};
static const DACConversionGroup dac_grpcfg = {
  .num_channels = 1U, .end_cb = NULL, .error_cb = NULL, .trigger = DAC_TRG(5)
};
static const GPTConfig gpt_cfg = {
  .frequency = GPT_HZ, .callback = NULL, .cr2 = TIM_CR2_MMS_1, .dier = 0U
};
static dacsample_t *const dac_src = (dacsample_t *)(FMC_FPGA_BASE + A_DAC);

// ---- pot mux (from test_pot_mux.c) ------------------------------------------
// PF8 landed on S1, PF9 on S0 (swapped from plan), so PF8=bit1, PF9=bit0
#define MUX_SEL_A_LINE PAL_LINE(GPIOF, 8U)
#define MUX_SEL_B_LINE PAL_LINE(GPIOF, 9U)
#define RELEASE_CH     2U    // osc2 release pot lives on channel 2, COM_B
#define COM_B          1U    // POTS group index of PC3 (see bsp_adc_config.c)

static void mux_select(uint8_t ch) {
  palWriteLine(MUX_SEL_A_LINE, (ch & 2U) ? PAL_HIGH : PAL_LOW);   // PF8 -> S1
  palWriteLine(MUX_SEL_B_LINE, (ch & 1U) ? PAL_HIGH : PAL_LOW);   // PF9 -> S0
}

#define SYNTH_WAVE 0U   // 0=sine 1=saw 2=square 3=triangle

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_pot_level: release pot (ch2) -> voice0 level, A440 ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();
  uint16_t magic = rd(CORE_MAGIC);
  bsp_printf("MAGIC = 0x%04X %s\n", (unsigned)magic, (magic == 0xACE1U) ? "OK" : "FAIL");

  // one always-on voice at A440 (note 69): inc = 440 * 2^32 / Fs
  uint32_t inc = (uint32_t)(440.0 * 4294967296.0 / (double)SAMPLE_RATE + 0.5);
  wr_freq32(A_VOICE_FREQ(0), inc);
  wr(A_VOICE_CTRL(0), voice_ctrl(1U, SYNTH_WAVE, 0U));   // gate on, level 0 to start

  // DAC on PA4, TIM6-triggered DMA straight from the FPGA dac register
  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
  dacStart(&DACD1, &dac_cfg);
  gptStart(&GPTD6, &gpt_cfg);
  dacStartConversion(&DACD1, &dac_grpcfg, dac_src, 1U);
  gptStartContinuous(&GPTD6, GPT_HZ / SAMPLE_RATE);

  // pot mux: park on the release channel, read its ADC common every loop
  palSetLineMode(MUX_SEL_A_LINE, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(MUX_SEL_B_LINE, PAL_MODE_OUTPUT_PUSHPULL);
  adc_init();
  adc_start_continuous(1U << ADC_GROUP_POTS);
  mux_select(RELEASE_CH);          // fixed selection; nothing else to scan yet

  bsp_printf("turn the pot - the A440 tone volume should follow it\r\n");

  uint16_t s[4];
  for (;;) {
    adc_get_samples(ADC_GROUP_POTS, s, 4U);
    uint16_t raw   = s[COM_B];                 // 0..65535 from the ch2 pot
    uint8_t  level = (uint8_t)(raw >> 8);      // 16-bit -> 8-bit level field
    wr(A_VOICE_CTRL(0), voice_ctrl(1U, SYNTH_WAVE, level));
    bsp_printf("pot=%5u -> level=%3u\r\n", (unsigned)raw, (unsigned)level);
    chThdSleepMilliseconds(50);
  }
}
