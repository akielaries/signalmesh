#include <math.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/ina219.h"

#define MASTER_SUCCESS_LED APM_BSP_OK
#define MASTER_ERROR_LED APM_BSP_ERR
#define SLAVE_ACTIVITY_LED APM_BSP_WARN

#define I2C_LOOPBACK_SLAVE_ADDR 0x52

// 100KHz from 50MHz PCLK
static const I2CConfig i2c_config = {.timingr = 0x00C0EAFF, // 100kHz timing
                                     .cr1     = 0,
                                     .cr2     = 0};

// Master Thread
static THD_WORKING_AREA(waMasterThread, 256);
static THD_FUNCTION(MasterThread, arg) {
  (void)arg;
  chRegSetThreadName("I2C Master");
  uint8_t tx_byte = 0xA0;
  uint8_t rx_byte;

  while (true) {
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1,
                                         I2C_LOOPBACK_SLAVE_ADDR,
                                         &tx_byte,
                                         1,
                                         &rx_byte,
                                         1,
                                         TIME_MS2I(100));

    if (msg == MSG_OK && rx_byte == tx_byte) {
      palToggleLine(MASTER_SUCCESS_LED);
    } else {
      bsp_printf("master err: %d\n", msg);
      i2cGetErrors(&I2CD1);
      palToggleLine(MASTER_ERROR_LED);
    }

    tx_byte++;
    chThdSleepMilliseconds(250); // Faster toggling
  }
}

// Slave Thread
static THD_WORKING_AREA(waSlaveThread, 256);
static THD_FUNCTION(SlaveThread, arg) {
  (void)arg;
  chRegSetThreadName("I2C Slave");
  uint8_t buffer[1];

  while (true) {
    msg_t rx_msg = i2cSlaveReceiveTimeout(&I2CD2, buffer, 1, TIME_INFINITE);

    if (rx_msg == MSG_OK) {
      palToggleLine(SLAVE_ACTIVITY_LED);
      msg_t tx_msg = i2cSlaveTransmitTimeout(&I2CD2, buffer, 1, TIME_INFINITE);
      if (tx_msg != MSG_OK) {
        // If slave tx fails, toggle the error led
        bsp_printf("slave err: %d\n", tx_msg);
        i2cGetErrors(&I2CD2);
        palToggleLine(MASTER_ERROR_LED);
      }
    }
  }
}

int main(void) {
  // System initializations.
  halInit();
  chSysInit();
  bsp_init();

  bsp_printf("--- Starting I2C Loopback Test ---\r\n");
  bsp_printf("I2C1 (Master) <--> I2C2 (Slave @ 0x%02X)\r\n",
             I2C_LOOPBACK_SLAVE_ADDR);
  bsp_printf(
      "Green LED: Master OK | Red LED: Error | Yellow LED: Slave RX\r\n");

  // Configure I2C1 (Master) pins: PB8 = SCL, PB9 = SDA
  palSetPadMode(
      GPIOB, 8, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);
  palSetPadMode(
      GPIOB, 9, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);

  // Configure I2C2 (Slave) pins: PF1 = SCL, PF0 = SDA
  palSetPadMode(
      GPIOF, 1, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);
  palSetPadMode(
      GPIOF, 0, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);

  // Enable I2C peripheral clocks.
  rccEnableI2C1(true);
  rccEnableI2C2(true);

  // Start I2C drivers
  i2cStart(&I2CD1, &i2c_config);
  i2cStart(&I2CD2, &i2c_config);

  // Configure I2CD2 as a slave device
  //i2cAcquireBus(&I2CD2);
  //I2CD2.i2c->OAR1 = (I2C_LOOPBACK_SLAVE_ADDR << 1) | I2C_OAR1_OA1EN;
  //i2cReleaseBus(&I2CD2);

  bsp_printf("I2C Peripherals started and configured.\r\n");

  // Start threads
  chThdCreateStatic(
      waSlaveThread, sizeof(waSlaveThread), NORMALPRIO + 2, SlaveThread, NULL);
  chThdCreateStatic(
      waMasterThread, sizeof(waMasterThread), NORMALPRIO + 1, MasterThread, NULL);

  // main() thread does nothing but sleep.
  while (true) {
    chThdSleepMilliseconds(1000);
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
