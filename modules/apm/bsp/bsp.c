#include "hal.h"
#include "ch.h"
#include "bsp/bsp.h"
#include "bsp/configs/bsp_uart_config.h" // For bsp_debug_stream and bsp_debug_uart_config
#include "bsp/utils/bsp_gpt.h"
#include "bsp/utils/bsp_io.h"
#include "drivers/driver_api.h"
#include "drivers/ina219.h"
#include "drivers/bme280.h"
#include "drivers/servo.h"
#include "drivers/driver_registry.h"
#include "bsp/configs/bsp_i2c_config.h" // For bsp_i2c_init()
#include <stddef.h>

// private data for ina219
static ina219_t ina219_dev_data;
// private data for bme280
// static bme280_t bme280_dev_data; // BME280 driver doesn't have a private data struct
// private data for servo
// static servo_t servo_dev_data; // Servo driver doesn't have a private data struct


// Array of devices present on APM
device_t board_devices[] = {
    {
        .name = "ina219", // This name must match the driver name
        .driver = &ina219_driver,
        .bus = &I2CD4, // Bus instance
        .priv = &ina219_dev_data, // Private data for the device
        .is_active = false
    },
    {
        .name = "bme280",
        .driver = &bme280_driver,
        .bus = &I2CD4,
        .priv = NULL,
        .is_active = false
    },
    {
        .name = "servo",
        .driver = &servo_driver,
        .bus = NULL,
        .priv = NULL,
        .is_active = false
    }
};

// Number of devices on APM
const size_t num_board_devices = sizeof(board_devices) / sizeof(board_devices[0]);

void bsp_init(void) {
  halInit();
  chSysInit();

  // Initialize the debug UART using bsp_io
  bsp_io_init();

  bsp_printf("booting...");
  for (uint8_t i = 0; i < 3; i++) {
    bsp_printf(".");
    chThdSleepMilliseconds(250);
  }
  bsp_printf("\n");

  init_devices();
  bsp_i2c_init();
  bsp_printf("\n...Starting...\n\n");
  bsp_printf("System tick freq: %u Hz\n", CH_CFG_ST_FREQUENCY);
  bsp_printf("DBGMCU->IDCODE: 0x%08lX\r\n", DBGMCU->IDCODE);
  bsp_printf("REV_ID: 0x%X\r\nDEV_ID: 0x%X\n", DBGMCU->IDCODE >> 16,
                                            DBGMCU->IDCODE & 0xFFF);

  /* ADC inputs.*/
  palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG); // PB1, ADC channel 5
  palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG); // PA3, ADC channel 15
  palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG); // PC0, ADC channel 10
  palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG); // PC3, ADC channel 13

  // PWM on PE11, timer 1 channel 2?
  palSetPadMode(GPIOE, 11, PAL_MODE_ALTERNATE(1));

  // Initialize GPT (from bsp_gpt.c)
  // bsp_gpt_init() should already be called by peripheral drivers
  // that rely on it, e.g. ADC, PWM.
  // For example, if ADC uses a GPT trigger, then adc_init() would call
  // bsp_gpt_init() It's not called directly here to avoid double initialization
  // or conflicts.

  // Initialize other peripherals (ADC, PWM) through their respective BSP init
  // functions adc_init(); // To be created if not already present pwm_init();
  // // To be created if not already present
}
