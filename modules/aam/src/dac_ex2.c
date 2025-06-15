#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>

#define AMPLITUDE  2047
#define OFFSET     2048
#define MAX_VOICES 4


typedef enum {
  NOTE_C0, NOTE_CS0, NOTE_D0, NOTE_DS0, NOTE_E0, NOTE_F0, NOTE_FS0, NOTE_G0, NOTE_GS0, NOTE_A0, NOTE_AS0, NOTE_B0,
  NOTE_C1, NOTE_CS1, NOTE_D1, NOTE_DS1, NOTE_E1, NOTE_F1, NOTE_FS1, NOTE_G1, NOTE_GS1, NOTE_A1, NOTE_AS1, NOTE_B1,
  NOTE_C2, NOTE_CS2, NOTE_D2, NOTE_DS2, NOTE_E2, NOTE_F2, NOTE_FS2, NOTE_G2, NOTE_GS2, NOTE_A2, NOTE_AS2, NOTE_B2,
  NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
  NOTE_C8, NOTE_CS8, NOTE_D8, NOTE_DS8, NOTE_E8, NOTE_F8, NOTE_FS8, NOTE_G8, NOTE_GS8, NOTE_A8, NOTE_AS8, NOTE_B8,
  NOTE_COUNT
} note_t;

// Frequency table (in Hz) for the corresponding note enum
static const float note_frequencies[NOTE_COUNT] = {
  16.35f, 17.32f, 18.35f, 19.45f, 20.60f, 21.83f, 23.12f, 24.50f, 25.96f, 27.50f, 29.14f, 30.87f,
  32.70f, 34.65f, 36.71f, 38.89f, 41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f,
  65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f,
  130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f,
  261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f,
  523.25f, 554.37f, 587.33f, 622.25f, 659.25f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, 987.77f,
  1046.50f, 1108.73f, 1174.66f, 1244.51f, 1318.51f, 1396.91f, 1479.98f, 1567.98f, 1661.22f, 1760.00f, 1864.66f, 1975.53f,
  2093.00f, 2217.46f, 2349.32f, 2489.02f, 2637.02f, 2793.83f, 2959.96f, 3135.96f, 3322.44f, 3520.00f, 3729.31f, 3951.07f,
  4186.01f, 4434.92f, 4698.63f, 4978.03f, 5274.04f, 5587.65f, 5919.91f, 6271.93f, 6644.88f, 7040.00f, 7458.62f, 7902.13f
};

void gpt6_callback(GPTDriver *gptp);

// Simple voice structure
typedef struct {
  float frequency;
  float phase;
  float amplitude;
  int waveform; // 0=sine, 1=square, 2=triangle, 3=sawtooth
  bool active;
} voice_t;

static voice_t voices[MAX_VOICES];
static BaseSequentialStream *chp = (BaseSequentialStream *)&SD5;

// DAC configuration (same as your original)
static const DACConfig dac_cfg = {
  .init     = 2048U,
  .datamode = DAC_DHRM_12BIT_RIGHT,
};

static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = USART_CR2_STOP1_BITS,
  .cr3   = 0,
};

// Your original dac_write function
void dac_write(int dac, int16_t val) {
  switch (dac) {
    case 1:
      dacPutChannelX(&DACD1, 0, val);
      break;
    case 2:
      dacPutChannelX(&DACD2, 0, val);
      break;
  }
}

// Generate waveform sample (copied from your code)
static float generate_sample(int waveform, float phase) {
  switch (waveform) {
    case 0: // Sine
      return sinf(phase);
    case 1: // Square
      return (sinf(phase) > 0) ? 1.0f : -1.0f;
    case 2: // Triangle
    {
      float normalized_phase = fmodf(phase, 2.0f * M_PI) / (2.0f * M_PI);
      return (normalized_phase < 0.5f) ? (4.0f * normalized_phase - 1.0f)
                                       : (3.0f - 4.0f * normalized_phase);
    }
    case 3: // Sawtooth
    {
      float normalized_phase = fmodf(phase, 2.0f * M_PI) / (2.0f * M_PI);
      return 2.0f * normalized_phase - 1.0f;
    }
    default:
      return 0.0f;
  }
}


CH_IRQ_HANDLER(TIM6_DAC_IRQHandler) {
  CH_IRQ_PROLOGUE();

  if (TIM6->SR & TIM_SR_UIF) {
    TIM6->SR &= ~TIM_SR_UIF;  // Clear update interrupt
    gpt6_callback(NULL);      // Call audio generator
  }

  CH_IRQ_EPILOGUE();
}

void gpt6_callback(GPTDriver *gptp) {
  (void)gptp;

  float mixed_sample = 0.0f;
  int active_count = 0;

  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].active) {
      float sample = generate_sample(voices[i].waveform, voices[i].phase);
      mixed_sample += sample * voices[i].amplitude;
      voices[i].phase += (2.0f * M_PI * voices[i].frequency) / 44100.0f;
      if (voices[i].phase >= 2.0f * M_PI)
        voices[i].phase -= 2.0f * M_PI;
      active_count++;
    }
  }

  if (active_count > 0) mixed_sample /= active_count;
  if (mixed_sample > 1.0f) mixed_sample = 1.0f;
  else if (mixed_sample < -1.0f) mixed_sample = -1.0f;

  dacPutChannelX(&DACD1, 0, (dacsample_t)(mixed_sample * AMPLITUDE + OFFSET));
  dacPutChannelX(&DACD2, 0, (dacsample_t)(mixed_sample * AMPLITUDE + OFFSET));
}

// Timer initialization (TIM6, 44.1kHz)
static void init_timer6_for_audio(uint32_t sample_rate_hz) {
  rccEnableTIM6(FALSE);
  rccResetTIM6();

  uint32_t timer_clk = STM32_TIMCLK1;
  uint32_t prescaler = 0;
  uint32_t period = (timer_clk / sample_rate_hz) - 1;

  TIM6->PSC = prescaler;
  TIM6->ARR = period;
  TIM6->DIER = TIM_DIER_UIE;
  TIM6->CR1 = TIM_CR1_CEN;
  TIM6->CNT = 0;

  nvicEnableVector(TIM6_DAC_IRQn, CORTEX_PRIO_MASK(12));
}

// Start a voice
int start_voice(float frequency, float amplitude, int waveform) {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (!voices[i].active) {
      voices[i].frequency = frequency;
      voices[i].phase     = 0.0f;
      voices[i].amplitude = amplitude;
      voices[i].waveform  = waveform;
      voices[i].active    = true;
      chprintf(chp, "Started voice %d: %.1f Hz\r\n", i, frequency);
      return i;
    }
  }
  chprintf(chp, "No free voices\r\n");
  return -1;
}

// Stop a voice
void stop_voice(int voice_id) {
  if (voice_id >= 0 && voice_id < MAX_VOICES) {
    voices[voice_id].active = false;
    chprintf(chp, "Stopped voice %d\r\n", voice_id);
  }
}

void test_c_major_chord(int waveform) {
  chprintf(chp, "\n=== C Major Chord ===\r\n");
  start_voice(note_frequencies[NOTE_C5], 0.15f, waveform);
  start_voice(note_frequencies[NOTE_E5], 0.15f, waveform);
  start_voice(note_frequencies[NOTE_G5], 0.15f, waveform);
  chThdSleepMilliseconds(3000);
  for (int i = 0; i < MAX_VOICES; i++) stop_voice(i);
}

void test_b_major_chord(int waveform) {
  chprintf(chp, "\n=== B Major Chord ===\r\n");
  start_voice(note_frequencies[NOTE_B4], 0.15f, waveform);
  start_voice(note_frequencies[NOTE_DS5], 0.15f, waveform);
  start_voice(note_frequencies[NOTE_FS5], 0.15f, waveform);
  chThdSleepMilliseconds(3000);
  for (int i = 0; i < MAX_VOICES; i++) stop_voice(i);
}

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Multi-Voice Test Starting...\r\n");

  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);

  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  dacStart(&DACD1, &dac_cfg);
  dacStart(&DACD2, &dac_cfg);

  chprintf(chp, "DAC initialized. Starting tests...\r\n");

  chprintf(chp, "DAC initialized. Starting TIM6 interrupt...\r\n");

  // Initialize timer for 44.1kHz audio output
  init_timer6_for_audio(44100);

  // Test chord (interrupt handles audio generation now)
  test_c_major_chord(1);
  chThdSleepMilliseconds(1000);
  test_b_major_chord(1);

  // LED blink loop
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  while (true) {
    palTogglePad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(500);
  }

  return 0;
}
