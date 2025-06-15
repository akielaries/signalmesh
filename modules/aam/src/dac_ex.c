#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <math.h>

#define SAMPLES   256 // Power of 2 for better performance
#define AMPLITUDE 2047
#define OFFSET    2048

static dacsample_t sine_table[SAMPLES];
BaseSequentialStream *chp = (BaseSequentialStream *)&SD5;

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

// Initialize sine table with better precision
void init_sine_table(void) {
  for (int i = 0; i < SAMPLES; i++) {
    double theta  = 2.0 * M_PI * i / SAMPLES; // Use double precision
    sine_table[i] = (dacsample_t)(AMPLITUDE * sin(theta) + OFFSET);
  }
}

// Alternative: Fixed sample rate with frequency control
void play_sine_wave_fixed_rate(float frequency_hz, uint32_t sample_rate_hz) {
  int i                 = 0;
  float phase           = 0.0f;
  float phase_increment = (2.0f * M_PI * frequency_hz) / sample_rate_hz;

  // Calculate sample interval for fixed sample rate
  uint32_t sample_interval_us     = 1000000U / sample_rate_hz;
  systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);
  systime_t next_time             = chVTGetSystemTimeX();

  chprintf(chp,
           "Playing %.1f Hz at %u Hz sample rate\r\n",
           frequency_hz,
           sample_rate_hz);
  chprintf(chp,
           "Phase increment: %.6f, Sample interval: %u us\r\n",
           phase_increment,
           sample_interval_us);

  while (true) {
    // Generate sample on-the-fly for exact frequency
    dacsample_t sample = (dacsample_t)(AMPLITUDE * sinf(phase) + OFFSET);

    // Output to both DAC channels
    dacPutChannelX(&DACD1, 0, sample);
    dacPutChannelX(&DACD2, 0, sample);

    // Advance phase
    phase += phase_increment;
    if (phase >= 2.0f * M_PI) {
      phase -= 2.0f * M_PI; // Keep phase in range
    }

    // Precise timing
    next_time += sample_interval_ticks;
    chThdSleepUntil(next_time);
  }
}

// High-resolution timing using RT thread
void play_sine_wave_rt(float frequency_hz) {
  int i = 0;

  // Calculate timing
  float samples_per_second        = frequency_hz * SAMPLES;
  uint32_t sample_interval_us     = (uint32_t)(1000000.0f / samples_per_second);
  systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);

  // Set this thread to high priority for better timing
  chThdSetPriority(HIGHPRIO);

  chprintf(chp, "Playing %.1f Hz sine wave (RT priority)\r\n", frequency_hz);

  systime_t next_time = chVTGetSystemTimeX();

  while (true) {
    // Output samples
    dacPutChannelX(&DACD1, 0, sine_table[i]);
    dacPutChannelX(&DACD2, 0, sine_table[i]);

    i = (i + 1) % SAMPLES;

    // High-precision timing
    next_time += sample_interval_ticks;
    chThdSleepUntil(next_time);
  }
}

// Generate different waveforms in lookup table
void generate_waveform(int waveform_type) {
  for (int i = 0; i < SAMPLES; i++) {
    float phase = 2.0f * M_PI * i / SAMPLES;
    dacsample_t sample;

    switch (waveform_type) {
      case 0: // Sine
        sample = (dacsample_t)(AMPLITUDE * sinf(phase) + OFFSET);
        break;
      case 1: // Square
        sample =
          (sinf(phase) > 0) ? (OFFSET + AMPLITUDE) : (OFFSET - AMPLITUDE);
        break;
      case 2: // Triangle
      {
        float t        = phase / (2.0f * M_PI);
        float triangle = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
        sample         = (dacsample_t)(AMPLITUDE * triangle + OFFSET);
      } break;
      case 3: // Sawtooth
      {
        float sawtooth = 2.0f * (phase / (2.0f * M_PI)) - 1.0f;
        sample         = (dacsample_t)(AMPLITUDE * sawtooth + OFFSET);
      } break;
      default:
        sample = OFFSET;
        break;
    }

    sine_table[i] = sample;
  }
}

// Your original DAC write function (kept for compatibility)
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

int main(void) {
  halInit();
  chSysInit();

  // UART Debug setup
  palSetPadMode(GPIOC, GPIOC_PIN12, PAL_MODE_ALTERNATE(8)); // USART5 TX
  palSetPadMode(GPIOD, GPIOD_PIN2, PAL_MODE_ALTERNATE(8));  // USART5 RX
  sdStart(&SD5, &uart5_cfg);
  chprintf(chp, "Wave Generator...\r\n");

  // GPIO Setup
  palSetPadMode(GPIOA, GPIOA_PIN3, PAL_STM32_MODE_OUTPUT); // LED
  palSetPadMode(GPIOA, GPIOA_PIN4, PAL_STM32_MODE_ANALOG); // DAC1_CH1
  palSetPadMode(GPIOA, GPIOA_PIN5, PAL_STM32_MODE_ANALOG); // DAC1_CH2

  // Startup LED sequence
  for (uint8_t i = 0; i < 5; i++) {
    palSetPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
    palClearPad(GPIOA, GPIOA_PIN3);
    chThdSleepMilliseconds(100);
  }

  // Initialize waveform table
  init_sine_table();

  // Start DAC channels
  dacStart(&DACD1, &dac_cfg);
  dacStart(&DACD2, &dac_cfg);

  chprintf(chp, "DAC initialized. Starting audio...\r\n");

  // Demo different frequencies with correct frequency generation
  float test_frequencies[]     = {100.0f, 220.0f, 440.0f, 880.0f};
  const char *waveform_names[] = {"Sine", "Square", "Triangle", "Sawtooth"};

  // Test different waveforms
  for (int w = 0; w < 4; w++) {
    chprintf(chp, "Testing %s waveform\r\n", waveform_names[w]);

    for (int f = 0; f < 4; f++) {
      float frequency_hz = test_frequencies[f];
      chprintf(chp,
               "Playing %s at %.1f Hz for 2 seconds\r\n",
               waveform_names[w],
               frequency_hz);

      // Method 1: Fixed sample rate with real-time calculation
      // This generates the correct frequency regardless of lookup table size
      float phase             = 0.0f;
      uint32_t sample_rate_hz = 44100; // Fixed sample rate
      float phase_increment   = (2.0f * M_PI * frequency_hz) / sample_rate_hz;

      uint32_t sample_interval_us     = 1000000U / sample_rate_hz;
      systime_t sample_interval_ticks = TIME_US2I(sample_interval_us);
      systime_t next_time             = chVTGetSystemTimeX();
      systime_t end_time = next_time + TIME_S2I(2); // Play for 2 seconds

      while (chVTGetSystemTimeX() < end_time) {
        dacsample_t sample;

        // Generate waveform sample based on current phase
        switch (w) {
          case 0: // Sine
            sample = (dacsample_t)(AMPLITUDE * sinf(phase) + OFFSET);
            break;
          case 1: // Square
            sample =
              (sinf(phase) > 0) ? (OFFSET + AMPLITUDE) : (OFFSET - AMPLITUDE);
            break;
          case 2: // Triangle
          {
            float normalized_phase = fmodf(phase, 2.0f * M_PI) / (2.0f * M_PI);
            float triangle         = (normalized_phase < 0.5f)
                                       ? (4.0f * normalized_phase - 1.0f)
                                       : (3.0f - 4.0f * normalized_phase);
            sample = (dacsample_t)(AMPLITUDE * triangle + OFFSET);
          } break;
          case 3: // Sawtooth
          {
            float normalized_phase = fmodf(phase, 2.0f * M_PI) / (2.0f * M_PI);
            float sawtooth         = 2.0f * normalized_phase - 1.0f;
            sample = (dacsample_t)(AMPLITUDE * sawtooth + OFFSET);
          } break;
          default:
            sample = OFFSET;
            break;
        }

        // Output to both DAC channels
        dacPutChannelX(&DACD1, 0, sample);
        dacPutChannelX(&DACD2, 0, sample);

        // Advance phase
        phase += phase_increment;
        if (phase >= 2.0f * M_PI) {
          phase -= 2.0f * M_PI;
        }

        // Precise timing
        next_time += sample_interval_ticks;
        chThdSleepUntil(next_time);
      }

      // Brief pause between frequencies
      chThdSleepMilliseconds(500);
    }

    // Pause between waveforms
    chThdSleepMilliseconds(1000);
  }

  chprintf(chp, "done...\r\n");

  // Infinite loop to keep system alive
  while (true) {
    palTogglePad(GPIOA, GPIOA_PIN3); // Blink LED to show system is alive
    chThdSleepSeconds(1);
  }

  return 0; // Unreachable
}
