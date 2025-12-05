#include <math.h>
#include "ch.h"
#include "hal.h"
#include "bsp/utils/bsp_print.h" // For printing utility
#include "drivers/adc.h"          // For ADC driver API
#include "portab.h"               // For PORTAB_LINE_BUTTON (temporary until button driver)


/*===========================================================================*/
/* ADC Conversion Formulas (retained as application logic)                   */
/*===========================================================================*/

#define TS_CAL1_ADDR            ((uint16_t*)0x1FF1E820)
#define TS_CAL2_ADDR            ((uint16_t*)0x1FF1E840)
#define VREFINT_CAL_ADDR        ((uint16_t*)0x1FF1E860)

// Parameters for a 10k NTC, Beta = 3950
#define THERMISTOR_NOMINAL   10000.0f
#define TEMPERATURE_NOMINAL  25.0f
#define B_COEFFICIENT        3950.0f
#define SERIES_RESISTOR      10000.0f

static float adc_to_steinhart(uint16_t adc_value) {
  float v_ratio = (float)adc_value / 65535.0f;
  float r_therm = SERIES_RESISTOR * (1.0f / v_ratio - 1.0f);

  float steinhart;
  steinhart = r_therm / THERMISTOR_NOMINAL;
  steinhart = logf(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0f / (TEMPERATURE_NOMINAL + 273.15f);
  steinhart = 1.0f / steinhart;
  steinhart -= 273.15f;
  return steinhart;
}

static float adc_to_vsense(uint16_t ts_data) {
  const float ts_cal1_temp = 30.0f;
  const float ts_cal2_temp = 130.0f;

  uint16_t ts_cal1 = *TS_CAL1_ADDR >> 4;
  uint16_t ts_cal2 = *TS_CAL2_ADDR >> 4;
  uint16_t ts_data_12bit = ts_data >> 4;

  float temp = ts_cal1_temp +
               ((float)(ts_data_12bit - ts_cal1)) *
                (ts_cal2_temp - ts_cal1_temp) /
                (float)(ts_cal2 - ts_cal1);
  return temp;
}

static float adc_to_vdda(uint16_t vref_data) {
  uint32_t vrefint_cal = (uint32_t)(*VREFINT_CAL_ADDR);
  if (vref_data == 0) return 0.0f;
  const float VREFINT_CAL_VOLT = 3.3f;
  float vdda = VREFINT_CAL_VOLT * ((float)vrefint_cal / (float)vref_data);
  return vdda;
}

/*===========================================================================*/
/* Test Application Code.                                                    */
/*===========================================================================*/

void run_test_adc(void) {
  adc_init(); // Initialize the ADC driver

  debug_printf("--- Starting ADC Test ---\n");
  debug_printf("Reading potentiometers, thermistor, and internal sensors.\n");
  debug_printf("Press button to stop.\n\n");

  // Start continuous conversions for both groups
  adc_start_continuous((1 << ADC_GROUP_POTS) | (1 << ADC_GROUP_INTERNAL));

  // Buffers to receive samples from the driver
  uint16_t pot_samples[4];
  uint16_t internal_samples[2];

  while (true) {
    // Get the latest samples from the ADC driver
    adc_get_samples(ADC_GROUP_POTS, pot_samples, 4);
    adc_get_samples(ADC_GROUP_INTERNAL, internal_samples, 2);

    // Extract individual values (based on how they are defined in bsp_adc_config.c)
    int pot1 = pot_samples[0];
    int pot2 = pot_samples[1];
    int pot3 = pot_samples[2];
    int therm1 = pot_samples[3];

    int v_sense = internal_samples[0];
    int v_ref = internal_samples[1];

    // Print values
    debug_printf("Pot1: %d  Pot2: %d Pot3: %d \n", pot1, pot2, pot3);
    debug_printf("Therm1: %d (%f C) V_SENSE: %d (%f C)\n\n",
             therm1,
             adc_to_steinhart(therm1),
             v_sense,
             adc_to_vsense(v_sense));
    debug_printf("VREF: %d VREF_CAL: %d VDDA: %f V\n",
                  v_ref,
                  *VREFINT_CAL_ADDR,
                  adc_to_vdda(v_ref));

    // Check for button press to stop the test
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      adc_stop_continuous((1 << ADC_GROUP_POTS) | (1 << ADC_GROUP_INTERNAL));
      debug_printf("ADC conversions stopped.\n");
      break;
    }
    chThdSleepMilliseconds(500);
  }
}
