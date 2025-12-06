#include "ch.h"
#include "hal.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/bsp.h" // For bsp_init()

// DAC configuration (same as your original)
static const DACConfig dac_cfg = {
  .init     = 2048U,
  .datamode = DAC_DHRM_12BIT_RIGHT,
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

void test_dac_write(int16_t right, int16_t left) {
  float dac1_volt = (right / (pow(2, 12)) * 3.3);
  float dac2_volt = (left / (pow(2, 12)) * 3.3);

  bsp_printf("writing %d (%f v) to DAC1 %d (%f v) to DAC2\n",
             right,
             dac1_volt,
             left,
             dac2_volt);
  dac_write(1, right);
  dac_write(2, left);
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
      bsp_printf("Started voice %d: %.1f Hz\n", i, frequency);
      return i;
    }
  }
  bsp_printf("No free voices\n");
  return -1;
}

// Stop a voice
void stop_voice(int voice_id) {
  if (voice_id >= 0 && voice_id < MAX_VOICES) {
    voices[voice_id].active = false;
    bsp_printf("Stopped voice %d\n", voice_id);
  }
}

// Test C major chord
void test_c_major_chord(int waveform) {
  bsp_printf("\n=== Testing C Major Chord ===\n");

  // Start three voices for C major chord (C, E, G)
  int voice1 = start_voice(note_frequencies[NOTE_C5], 0.3f, waveform); // C4
  int voice2 = start_voice(note_frequencies[NOTE_E5], 0.3f, waveform); // E4
  int voice3 = start_voice(note_frequencies[NOTE_G5], 0.3f, waveform); // G4

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

    // something about clamping the signal to prevent distortion but this
    // doesn't work!
    if (active_count > 0) {
      mixed_sample /= active_count;
    }
    if (mixed_sample > 1.0f)
      mixed_sample = 1.0f;
    if (mixed_sample < -1.0f)
      mixed_sample = -1.0f;

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

  bsp_printf("cmaj test complete\n");
}

void test_b_major_chord(int waveform) {
  bsp_printf("\n=== Testing B Major Chord ===\n");

  int voice1 = start_voice(note_frequencies[NOTE_B4], 0.3f, waveform);
  int voice2 = start_voice(note_frequencies[NOTE_DS5], 0.3f, waveform);
  int voice3 = start_voice(note_frequencies[NOTE_FS5], 0.3f, waveform);

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

    // something about clamping the signal to prevent distortion but this
    // doesn't work!
    if (active_count > 0) {
      mixed_sample /= active_count;
    }
    if (mixed_sample > 1.0f)
      mixed_sample = 1.0f;
    if (mixed_sample < -1.0f)
      mixed_sample = -1.0f;

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

  bsp_printf("bmaj test complete\n");
}

bsp_init();
bsp_printf("Multi-Voice Test Starting...\n");
bsp_printf("System tick freq: %u Hz\n", CH_CFG_ST_FREQUENCY);

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

bsp_printf("DAC initialized. Starting tests...\n");

test_dac_write(2048, 2048);
chThdSleepSeconds(5);


bsp_printf("Testing single 440Hz sine wave...\n");
float frequency_hz      = 440.0f;
float phase             = 0.0f;
uint32_t sample_rate_hz = 44100;
float phase_increment   = (2.0f * M_PI * frequency_hz) / sample_rate_hz;

uint32_t sample_interval_us     = 1000000U / sample_rate_hz;
systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);
systime_t next_time             = chVTGetSystemTimeX();
systime_t end_time              = next_time + TIME_S2I(60);

bsp_printf("ticks: %d\n", sample_interval_ticks);
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

bsp_printf("Single tone test complete\n");
chThdSleepMilliseconds(500);

while (true) {
  test_c_major_chord(0);
  test_b_major_chord(0);
}


while (true) {
  palTogglePad(GPIOA, GPIOA_PIN3);
  chThdSleepSeconds(1);
}

return 0;
}
