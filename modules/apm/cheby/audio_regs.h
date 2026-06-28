#ifndef __CHEBY__AUDIO__H__
#define __CHEBY__AUDIO__H__

#define AUDIO_SIZE 128 /* 0x80 */

/* REG ctrl */
#define AUDIO_CTRL 0x0UL
#define AUDIO_CTRL_ENABLE 0x1UL
#define AUDIO_CTRL_ENABLE_MASK 0x1UL
#define AUDIO_CTRL_ENABLE_SHIFT 0
#define AUDIO_CTRL_SRATE_MASK 0x6UL
#define AUDIO_CTRL_SRATE_SHIFT 1

/* REG status */
#define AUDIO_STATUS 0x2UL
#define AUDIO_STATUS_ACTIVE_VOICES_MASK 0xffUL
#define AUDIO_STATUS_ACTIVE_VOICES_SHIFT 0

/* REG sample */
#define AUDIO_SAMPLE 0x4UL

/* REG dac */
#define AUDIO_DAC 0x6UL

/* REG voice */
#define AUDIO_VOICE 0x40UL
#define AUDIO_VOICE_SIZE 8 /* 0x8 */

/* REG freq */
#define AUDIO_VOICE_FREQ 0x0UL

/* REG ctrl */
#define AUDIO_VOICE_CTRL 0x4UL
#define AUDIO_VOICE_CTRL_GATE 0x1UL
#define AUDIO_VOICE_CTRL_GATE_MASK 0x1UL
#define AUDIO_VOICE_CTRL_GATE_SHIFT 0
#define AUDIO_VOICE_CTRL_WAVE_MASK 0xeUL
#define AUDIO_VOICE_CTRL_WAVE_SHIFT 1
#define AUDIO_VOICE_CTRL_LEVEL_MASK 0xff00UL
#define AUDIO_VOICE_CTRL_LEVEL_SHIFT 8

#ifndef __ASSEMBLER__
struct audio {
  /* [0x0]: REG (rw) */
  uint16_t ctrl;

  /* [0x2]: REG (ro) */
  uint16_t status;

  /* [0x4]: REG (ro) */
  uint16_t sample;

  /* [0x6]: REG (ro) */
  uint16_t dac;

  /* padding to: 64 Bytes */
  uint32_t __padding_0[14];

  /* [0x40]: REPEAT */
  struct voice {
    /* [0x0]: REG (rw) */
    uint32_t freq;

    /* [0x4]: REG (rw) */
    uint16_t ctrl;

    /* padding to: 8 Bytes */
    uint8_t __padding_0[2];
  } voice[8];
};
#endif /* !__ASSEMBLER__*/

#endif /* __CHEBY__AUDIO__H__ */
