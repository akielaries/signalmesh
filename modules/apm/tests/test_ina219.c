#include <math.h>
#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/configs/bsp_i2c_config.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/ina219.h"


int main() {
  bsp_init();

  bsp_printf("--- Starting I2C Debug Test ---\r\n");
  bsp_printf("Testing I2C startup step by step\r\n");
  bsp_printf("Press button to stop.\r\n\r\n");

  // Step 1: Test I2C2 only (your original working config)
  //i2cStop(&I2CD2);
  //bsp_printf("stopped I2C2\n");

  // 100KHz from 50MHz PCLK
  static const I2CConfig i2c1_config = {.timingr = 0x10707dbc, // 100kHz timing
                                        .cr1     = 0,
                                        .cr2     = 0};
  static const I2CConfig i2c2_config = {.timingr = 0x00C0EAFF, // 100kHz timing
                                        .cr1     = 0,
                                        .cr2     = 0};

  // Step 2: Try I2C1 GPIO config only (no start yet)
  bsp_printf("Configuring I2C1 GPIO...\r\n");
  palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(4)); // PB6 = I2C1_SCL
  palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(4)); // PB7 = I2C1_SDA
  bsp_printf("I2C1 GPIO configured\r\n");

  bsp_printf("Starting I2C1 only...\n");
  chThdSleepMilliseconds(100);
  rccEnableI2C1(true);
  i2cStart(&I2CD1, &i2c1_config);
  bsp_printf("I2C1 started successfully\r\n");

  // Small delay
  chThdSleepMilliseconds(100);

  bsp_printf("Starting I2C2...\r\n");
  rccEnableI2C2(true);
  i2cStart(&I2CD2, &i2c2_config);
  bsp_printf("I2C2 started successfully\r\n");

  // If we get here, both I2C drivers are working
  bsp_printf("Both I2C drivers started successfully!\r\n");

  // Loopback test
  while (true) {
    static uint8_t test_data = 0x55; // Alternating pattern
    uint8_t rx_data[2];

    // I2C1 as master, I2C2 as slave (or vice versa)
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1,
                                         0x52,
                                         &test_data,
                                         1,
                                         rx_data,
                                         1,
                                         TIME_MS2I(1000));

    if (msg == MSG_OK) {
      bsp_printf("I2C1->I2C2: TX:0x%02X RX:0x%02X\r\n", test_data, rx_data[0]);
    } else {
      bsp_printf("I2C Error: %d\r\n", msg);
    }

    test_data++; // Change test data each time

    // Check for button press to stop
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("Test stopped.\r\n");
      break;
    }

    chThdSleepMilliseconds(500);
  }

  bsp_printf("starting i2c driver\n");
  chThdSleepMilliseconds(100);

  i2cStart(&I2CD2, &bsp_i2c_config);
  bsp_printf("i2c driver started\n");


  while (true) {
    unsigned i;
    msg_t msg;
    static const uint8_t cmd[] = {0, 0};
    uint8_t data[16];

    msg = i2cMasterTransmitTimeout(&I2CD2,
                                   0x52,
                                   cmd,
                                   sizeof(cmd),
                                   data,
                                   sizeof(data),
                                   TIME_INFINITE);
    if (msg != MSG_OK) {
      bsp_printf("bad xmit\n");
    }

    for (i = 0; i < 256; i++) {
      msg = i2cMasterReceiveTimeout(&I2CD2,
                                    0x52,
                                    data,
                                    sizeof(data),
                                    TIME_INFINITE);
      if (msg != MSG_OK) {
        bsp_printf("bad recv\n");
      }
    }
    chThdSleepMilliseconds(500);
  }

  return 0;
}

int foo(void) {
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
    } else {
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

