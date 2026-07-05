// auto-gated ADSR envelope on one A440 voice - for scoping the envelope shape.
// retriggers a note every ~3 s (gate on 1.5 s, then release) and animates voice0's
// `level` with a ~1 kHz firmware ADSR. A/D/S/R come from the osc2 pots (all COM_B):
//   ch0 = Attack (pin 1), ch1 = Decay (pin 5), ch3 = Sustain (pin 4), ch2 = Release (pin 2)
// scope the DAC output (PA4): the A440 carrier's amplitude follows the envelope.
// move a slider -> watch that segment of the shape change.
//
// unwired channels float, so their segment will be erratic until you wire that pot
// (or ground the channel). wire the ones you're testing.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/adc.h"
#include "cheby/core_regs.h"
#include "cheby/audio_regs.h"

// ---- FMC + audio register access (from test_pot_level.c) ---------------------
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
  wr(base + 0, (uint16_t)(f >> 16));
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

// ---- DAC streaming (from test_pot_level.c) ----------------------------------
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

// ---- pot mux ----------------------------------------------------------------
// PF8 landed on S1, PF9 on S0 (swapped from plan): PF8=bit1, PF9=bit0
#define MUX_SEL_A_LINE PAL_LINE(GPIOF, 8U)
#define MUX_SEL_B_LINE PAL_LINE(GPIOF, 9U)
#define COM_B 1U            // POTS group index of PC3 (osc2 bank)

// osc2 ADSR pot -> mux channel
#define CH_ATK 0U
#define CH_DEC 1U
#define CH_REL 2U
#define CH_SUS 3U

static void mux_select(uint8_t ch) {
  palWriteLine(MUX_SEL_A_LINE, (ch & 2U) ? PAL_HIGH : PAL_LOW);   // PF8 -> S1
  palWriteLine(MUX_SEL_B_LINE, (ch & 1U) ? PAL_HIGH : PAL_LOW);   // PF9 -> S0
}

// ---- envelope ---------------------------------------------------------------
enum { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

// map a 0..65535 pot to a time in [1, max_ms]
static uint32_t map_ms(uint16_t raw, uint32_t max_ms) {
  return 1U + (uint32_t)raw * (max_ms - 1U) / 65535U;
}

#define SYNTH_WAVE 2U       // 0=sine 1=saw 2=square 3=triangle
#define HOLD_MS    400U    // gate held on
#define PERIOD_MS  3000U    // full retrigger cycle

// all four ADSR sliders are (being) wired - every stage reads live from its pot

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_env_autogate: firmware ADSR on voice0, retrigger ~3s ---\r\n");

  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();
  uint16_t magic = rd(CORE_MAGIC);
  bsp_printf("MAGIC = 0x%04X %s\n", (unsigned)magic, (magic == 0xACE1U) ? "OK" : "FAIL");

  // voice0 at A440, level animated by the envelope below
  uint32_t inc = (uint32_t)(440.0 * 4294967296.0 / (double)SAMPLE_RATE + 0.5);
  wr_freq32(A_VOICE_FREQ(0), inc);
  wr(A_VOICE_CTRL(0), voice_ctrl(0U, SYNTH_WAVE, 0U));

  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
  dacStart(&DACD1, &dac_cfg);
  gptStart(&GPTD6, &gpt_cfg);
  dacStartConversion(&DACD1, &dac_grpcfg, dac_src, 1U);
  gptStartContinuous(&GPTD6, GPT_HZ / SAMPLE_RATE);

  palSetLineMode(MUX_SEL_A_LINE, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(MUX_SEL_B_LINE, PAL_MODE_OUTPUT_PUSHPULL);
  adc_init();
  adc_start_continuous(1U << ADC_GROUP_POTS);

  bsp_printf("scope PA4; move a slider to reshape that segment\r\n");

  uint16_t pot[4]   = {0U, 0U, 0U, 0U};   // latest per-channel reading (COM_B)
  uint8_t  scan_ch  = 0U;
  mux_select(scan_ch);

  int      env      = ENV_IDLE;
  float    lvl      = 0.0f;                // 0..255
  float    rel_step = 0.0f;                // captured at note-off
  uint32_t t        = 0U;                  // ms within the gate cycle
  uint32_t dbg      = 0U;

  for (;;) {
    // pipelined mux scan: read the channel selected last tick (settled), advance
    uint16_t s[4];
    adc_get_samples(ADC_GROUP_POTS, s, 4U);
    pot[scan_ch] = s[COM_B];
    scan_ch = (scan_ch + 1U) & 3U;
    mux_select(scan_ch);

    // all four stages live from their pots
    float sus      = (float)(pot[CH_SUS] >> 8);                          // 0..255
    float atk_step = 255.0f / (float)map_ms(pot[CH_ATK], 1000U);
    float dec_step = (255.0f - sus) / (float)map_ms(pot[CH_DEC], 1000U);

    // auto-gate transitions
    if (t == 0U) {
      env = ENV_ATTACK;
    }
    if (t == HOLD_MS && env != ENV_IDLE) {
      env = ENV_RELEASE;
      rel_step = lvl / (float)map_ms(pot[CH_REL], 1500U);         // ramp to 0 over rel time
      if (rel_step <= 0.0f) { rel_step = 0.5f; }
    }

    // advance 1 ms
    switch (env) {
      case ENV_ATTACK:
        lvl += atk_step;
        if (lvl >= 255.0f) { lvl = 255.0f; env = ENV_DECAY; }
        break;
      case ENV_DECAY:
        lvl -= dec_step;
        if (lvl <= sus) { lvl = sus; env = ENV_SUSTAIN; }
        break;
      case ENV_SUSTAIN:
        lvl = sus;
        break;
      case ENV_RELEASE:
        lvl -= rel_step;
        if (lvl <= 0.0f) { lvl = 0.0f; env = ENV_IDLE; }
        break;
      default:
        lvl = 0.0f;
        break;
    }

    uint8_t level = (uint8_t)(lvl + 0.5f);
    uint8_t gate  = (env == ENV_IDLE) ? 0U : 1U;
    wr(A_VOICE_CTRL(0), voice_ctrl(gate, SYNTH_WAVE, level));

    if (++dbg >= 250U) {                    // ~4 Hz status
      dbg = 0U;
      bsp_printf("A=%5u D=%5u S=%5u R=%5u  env=%d lvl=%3u\r\n",
                 (unsigned)pot[CH_ATK], (unsigned)pot[CH_DEC],
                 (unsigned)pot[CH_SUS], (unsigned)pot[CH_REL],
                 env, (unsigned)level);
    }

    if (++t >= PERIOD_MS) { t = 0U; }
    chThdSleepMilliseconds(1);
  }
}
