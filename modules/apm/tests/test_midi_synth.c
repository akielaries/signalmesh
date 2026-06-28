// minimal MIDI synth: USB-MIDI keyboard -> FPGA DDS voice -> DAC/PA4.
//
// ties together the working pieces:
//   USB host (OTG_FS, PA11/PA12)  -> usbh_midi note events
//   note -> tuning word           -> acm_top audio voice0 over FMC
//   FPGA dds_synth                -> `sample` register
//   GPT @ Fs reads sample         -> DAC1/PA4
//
// monophonic, notes only (velocity -> level is free, so it's wired). play a key,
// hear it on PA4. one voice for now; polyphony/voice-allocation comes later.

#include <math.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "usbh_midi.h"
#include "cheby/core_regs.h"
#include "cheby/audio_regs.h"

// ---- FMC + audio register access (from test_fmc_dds_e2e.c) --------------------
#define FMC_FPGA_BASE 0x60000000UL
#define AUDIO_BASE    0x80UL

static inline uint16_t rd(uint32_t off) {
  return *(volatile uint16_t *)(FMC_FPGA_BASE + off);
}
static inline void wr(uint32_t off, uint16_t v) {
  *(volatile uint16_t *)(FMC_FPGA_BASE + off) = v;
}

#define A_SAMPLE        (AUDIO_BASE + AUDIO_SAMPLE)
#define A_DAC           (AUDIO_BASE + AUDIO_DAC)     // FPGA 12-bit DAC-ready code
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

// ---- DAC streaming (from test_fmc_dds_e2e.c) ---------------------------------
#define SAMPLE_RATE   48000U
#define GPT_HZ        1200000U

static const DACConfig dac_cfg = {
  .init = 2048U,                          // mid-scale (AC zero) until streaming
  .datamode = DAC_DHRM_12BIT_RIGHT,
  .cr = 0U
};

// TIM6-triggered DMA plays directly from the FPGA's `dac` register - no CPU in
// the audio path. the FPGA already produced a DAC-ready 12-bit code, so the DMA
// just moves FMC -> DAC each Fs tick.
static const DACConversionGroup dac_grpcfg = {
  .num_channels = 1U,
  .end_cb       = NULL,
  .error_cb     = NULL,
  .trigger      = DAC_TRG(5)              // TIM6 TRGO (TSEL=5 on H7)
};

// TIM6 as the Fs trigger source: TRGO on update event (no callback - DMA does it)
static const GPTConfig gpt_cfg = {
  .frequency = GPT_HZ, .callback = NULL, .cr2 = TIM_CR2_MMS_1, .dier = 0U
};

// depth-1 circular "buffer" = the FPGA dac register; the DMA re-reads it each
// trigger (non-cacheable FMC, so it always sees the live value).
static dacsample_t *const dac_src = (dacsample_t *)(FMC_FPGA_BASE + A_DAC);

#define SYNTH_WAVE  0U                  // 0=sine 1=saw 2=square 3=triangle
#define NVOICES     8U                  // matches audio_regs voice[8]

static uint32_t note_inc[128];          // MIDI note -> DDS tuning word
static uint8_t  voice_note[NVOICES];    // MIDI note each voice plays, 0xFF = free
static uint8_t  steal_next = 0U;        // round-robin victim when all voices busy

static void build_note_table(void) {
  // inc = note_freq * 2^32 / Fs ; note_freq = 440 * 2^((n-69)/12).
  // using SAMPLE_RATE for both this table and the DAC rate cancels the small
  // FPGA-Fs(48043)-vs-DAC(48000) difference, so pitch lands ~right at the output.
  for (int n = 0; n < 128; n++) {
    double hz = 440.0 * pow(2.0, (n - 69) / 12.0);
    note_inc[n] = (uint32_t)(hz * 4294967296.0 / (double)SAMPLE_RATE + 0.5);
  }
}

static void synth_all_off(void) {
  for (uint8_t i = 0; i < NVOICES; i++) {
    voice_note[i] = 0xFFU;
    wr(A_VOICE_CTRL(i), voice_ctrl(0U, SYNTH_WAVE, 0U));
  }
}

// polyphonic note handler: one DDS voice per held note, FPGA mixes all 8.
// called from the USB-MIDI driver on each note event (IN-completion context).
static void note_cb(uint8_t note, uint8_t vel, bool on) {
  note &= 0x7FU;
  if (on) {
    uint8_t v = 0xFFU;                          // grab a free voice...
    for (uint8_t i = 0; i < NVOICES; i++) {
      if (voice_note[i] == 0xFFU) { v = i; break; }
    }
    if (v == 0xFFU) {                           // ...or steal one (round-robin)
      v = steal_next;
      steal_next = (uint8_t)((steal_next + 1U) % NVOICES);
    }
    voice_note[v] = note;
    wr_freq32(A_VOICE_FREQ(v), note_inc[note]);
    wr(A_VOICE_CTRL(v), voice_ctrl(1U, SYNTH_WAVE, (uint8_t)(vel << 1)));
  }
  else {
    for (uint8_t i = 0; i < NVOICES; i++) {     // release the voice on this note
      if (voice_note[i] == note) {
        wr(A_VOICE_CTRL(i), voice_ctrl(0U, SYNTH_WAVE, 0U));
        voice_note[i] = 0xFFU;
        break;
      }
    }
  }
}

// ---- USB debug sink (from midi_usb_enum_test.c) ------------------------------
extern SerialDriver *const bsp_debug_uart_driver;
void usbh_debug_output(const uint8_t *buff, size_t len) {
  sdWrite(bsp_debug_uart_driver, buff, len);
  sdWrite(bsp_debug_uart_driver, (const uint8_t *)"\r\n", 2);
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_midi_synth: USB-MIDI -> DDS -> DAC/PA4 ---\r\n");

  // FPGA register bus + sanity
  fmc_gpio_init();
  fmc_ctrl_init();
  fmc_mpu_init();
  uint16_t magic = rd(CORE_MAGIC);
  bsp_printf("MAGIC = 0x%04X %s\n", (unsigned)magic, (magic == 0xACE1U) ? "OK" : "FAIL");

  build_note_table();
  synth_all_off();          // all 8 voices gated off to start

  // DAC on PA4, fed by TIM6-triggered DMA straight from the FPGA dac register
  palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
  dacStart(&DACD1, &dac_cfg);
  gptStart(&GPTD6, &gpt_cfg);
  dacStartConversion(&DACD1, &dac_grpcfg, dac_src, 1U);   // depth 1 = re-read FMC each tick
  gptStartContinuous(&GPTD6, GPT_HZ / SAMPLE_RATE);       // 25 -> 48 kHz TRGO

  // hand parsed notes to the synth glue
  usbhmidiSetNoteCallback(note_cb);

  // USB host on OTG_FS (PA11/PA12) - same bring-up as midi_usb_enum_test.c
  palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10) | PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10) | PAL_STM32_OSPEED_HIGHEST);
  usbhStart(&USBHD2);
  volatile uint32_t *const otg_fs_hcfg = (volatile uint32_t *)0x40080400UL;
  volatile uint32_t *const otg_gotgctl = (volatile uint32_t *)0x40080000UL;
  *otg_fs_hcfg = (*otg_fs_hcfg & ~0x3u) | 0x1u;            // 48 MHz FS host clock
  *otg_gotgctl |= (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5);  // VBUS/session valid

  bsp_printf("plug in the keyboard and play (polyphonic, %u voices)...\n",
             (unsigned)NVOICES);

  uint32_t tick = 0;
  for (;;) {
    usbhMainLoop(&USBHD2);
    chThdSleepMilliseconds(5);
    if (++tick >= 200U) {                // ~1 Hz: re-assert FS clock once CMOD=1
      tick = 0;
      *otg_fs_hcfg = (*otg_fs_hcfg & ~0x3u) | 0x1u;
    }
  }
}
