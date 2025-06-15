#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>

#define AMPLITUDE  2047
#define OFFSET     2048
#define MAX_VOICES 4

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

// Test C major chord
void test_c_major_chord(int waveform) {
  chprintf(chp, "\n=== Testing C Major Chord ===\r\n");

  // Start three voices for C major chord (C, E, G)
  int voice1 = start_voice(261.63f, 0.3f, waveform); // C4
  int voice2 = start_voice(329.63f, 0.3f, waveform); // E4
  int voice3 = start_voice(392.00f, 0.3f, waveform); // G4

  // Play for a while using your original timing approach
  uint32_t sample_rate_hz         = 44100;
  uint32_t sample_interval_us     = 1000000U / sample_rate_hz;
  systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);
  systime_t next_time             = chVTGetSystemTimeX();
  systime_t end_time              = next_time + TIME_S2I(3); // 3 seconds

  while (chVTGetSystemTimeX() < end_time) {
    // Mix all active voices
    float mixed_sample = 0.0f;
    int active_count   = 0;

    for (int i = 0; i < MAX_VOICES; i++) {
      if (voices[i].active) {
        float sample = generate_sample(voices[i].waveform, voices[i].phase);
        mixed_sample += sample * voices[i].amplitude;
        active_count++;

        // Advance phase
        float phase_increment =
          (2.0f * M_PI * voices[i].frequency) / sample_rate_hz;
        voices[i].phase += phase_increment;
        if (voices[i].phase >= 2.0f * M_PI) {
          voices[i].phase -= 2.0f * M_PI;
        }
      }
    }

    // Convert to DAC sample
    dacsample_t dac_sample = (dacsample_t)(mixed_sample * AMPLITUDE + OFFSET);

    // Output to both DACs (same as your original)
    dacPutChannelX(&DACD1, 0, dac_sample);
    dacPutChannelX(&DACD2, 0, dac_sample);

    // Your original timing
    next_time += sample_interval_ticks;
    chThdSleepUntil(next_time);
  }

  // Stop all voices
  stop_voice(voice1);
  stop_voice(voice2);
  stop_voice(voice3);

  chprintf(chp, "Chord test complete\r\n");
}

int main(void) {
  halInit();
  chSysInit();

  // UART Debug setup (same as your original)
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8));
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Multi-Voice Test Starting...\r\n");

  // GPIO Setup (same as your original)
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT);
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG);
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG);

  // Startup LED sequence (same as your original)
  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  // Start DAC channels (same as your original)
  dacStart(&DACD1, &dac_cfg);
  dacStart(&DACD2, &dac_cfg);

  chprintf(chp, "DAC initialized. Starting tests...\r\n");

  // First, test single sine wave like your original code
  chprintf(chp, "Testing single 440Hz sine wave...\r\n");
  float frequency_hz      = 440.0f;
  float phase             = 0.0f;
  uint32_t sample_rate_hz = 44100;
  float phase_increment   = (2.0f * M_PI * frequency_hz) / sample_rate_hz;

  uint32_t sample_interval_us     = 1000000U / sample_rate_hz;
  systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);
  systime_t next_time             = chVTGetSystemTimeX();
  systime_t end_time              = next_time + TIME_S2I(2); // 2 seconds

  while (chVTGetSystemTimeX() < end_time) {
    dacsample_t sample = (dacsample_t)(AMPLITUDE * sinf(phase) + OFFSET);

    dacPutChannelX(&DACD1, 0, sample);
    dacPutChannelX(&DACD2, 0, sample);

    phase += phase_increment;
    if (phase >= 2.0f * M_PI) {
      phase -= 2.0f * M_PI;
    }

    next_time += sample_interval_ticks;
    chThdSleepUntil(next_time);
  }

  chprintf(chp, "Single tone test complete\r\n");
  chThdSleepMilliseconds(500);

  // Now test chord
  test_c_major_chord(0); // Sine wave chord


  // Keep system alive
  while (true) {
    palTogglePad(GPIOA, GPIOA_PIN3);
    chThdSleepSeconds(1);
  }

  return 0;
}
