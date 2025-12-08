#include <math.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/ina219.h"


int main(void) {
  bsp_init();

  bsp_printf("--- Starting INA219 Test ---\r\n");
  bsp_printf("Testing INA219 current/power sensor.\r\n");
  bsp_printf("Press button to stop.\r\n\r\n");
  chThdSleepMilliseconds(100);

  // Initialize I2C interface
  ina219_init();

  // Configure INA219 device
  ina219_t ina219_dev;
  ina219_config_t config = {.i2c_address      = INA219_DEFAULT_ADDRESS,
                            .shunt_resistance = 0.1f, // 0.1 ohm shunt resistor
                            .bus_range        = INA219_BUS_RANGE_32V,
                            .gain             = INA219_GAIN_1_40MV,
                            .bus_adc_resolution   = INA219_ADC_12BIT,
                            .shunt_adc_resolution = INA219_ADC_12BIT,
                            .mode                 = INA219_MODE_SHUNT_BUS_CONT};

  // Initialize the device
  if (!ina219_device_init(&ina219_dev, &config)) {
    bsp_printf("Failed to initialize INA219 device!\n");
    return 0;
  }

  bsp_printf("INA219 device initialized successfully.\n");
  bsp_printf("Shunt resistance: %.3f ohm\n", config.shunt_resistance);
  bsp_printf("I2C address: 0x%02X\n\n", config.i2c_address);

  // Main measurement loop
  while (true) {
    float shunt_voltage_mv, bus_voltage_v, current_ma, power_mw;

    // Read all measurements
    if (ina219_read_all(&ina219_dev,
                        &shunt_voltage_mv,
                        &bus_voltage_v,
                        &current_ma,
                        &power_mw)) {
      bsp_printf("Bus Voltage: %.2f V\n", bus_voltage_v);
      bsp_printf("Shunt Voltage: %.3f mV\n", shunt_voltage_mv);
      bsp_printf("Current: %.3f mA\n", current_ma);
      bsp_printf("Power: %.3f mW\n", power_mw);

      // Calculate load voltage (bus voltage + shunt voltage drop)
      float load_voltage_v = bus_voltage_v + (shunt_voltage_mv / 1000.0f);
      bsp_printf("Load Voltage: %.2f V\n", load_voltage_v);

      // Calculate resistance if current is flowing
      if (fabsf(current_ma) > 0.1f) {
        float load_ohms = load_voltage_v / (current_ma / 1000.0f);
        bsp_printf("Load Resistance: %.2f ohm\n", load_ohms);
      }
    }
    else {
      bsp_printf("Failed to read INA219 measurements!\n");
    }

    bsp_printf("----------------------------------------\n");

    // Check for button press to stop the test
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("INA219 test stopped.\n");
      break;
    }

    chThdSleepMilliseconds(1000);
  }

  return 0;
}

