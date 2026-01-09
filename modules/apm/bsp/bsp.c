#include <stddef.h>

#include "hal.h"
#include "ch.h"

#include "bsp/bsp.h"

#include "bsp/utils/bsp_gpt.h"
#include "bsp/utils/bsp_io.h"

#include "bsp/configs/bsp_uart_config.h" // for bsp_debug_stream and bsp_debug_uart_config
#include "bsp/configs/bsp_i2c_config.h" // for bsp_i2c_init()
#include "bsp/configs/bsp_spi_config.h" // for bsp_spi_init()

#include "drivers/driver_registry.h"
#include "drivers/driver_api.h"

#include "drivers/ina219.h"
#include "drivers/ina3221.h"
#include "drivers/bme280.h"
#include "drivers/bh1750.h"
#include "drivers/aht2x.h"
#include "drivers/24lc256.h"
#include "drivers/w25qxx.h"
#include "drivers/servo.h"



// private data for ina219
static ina219_t ina219_dev_data;
// private data for ina3221
static ina3221_t ina3221_dev_data;
// private data for bme280
// static bme280_t bme280_dev_data; // BME280 driver doesn't have a private data struct
// private data for bh1750
static bh1750_t bh1750_dev_data;
static aht2x_t aht2x_dev_data;
static eeprom_24lc256_t eeprom_24lc256_dev_data;
// private data for w25qxx
static w25qxx_t w25qxx_dev_data;
// private data for servo
// static servo_t servo_dev_data; // servo driver doesn't have a private data struct


// array of devices present on APM
device_t board_devices[] = {
    {
        .name = "ina219", // this name must match the driver name
        .driver = &ina219_driver,
        .bus = &I2CD4, // bus instance
        .priv = &ina219_dev_data, // private data for the device
        .is_active = false
    },
    {
        .name = "ina3221",
        .driver = &ina3221_driver,
        .bus = &I2CD4,
        .priv = &ina3221_dev_data,
        .is_active = false
    },
    {
        .name = "aht2x",
        .driver = &aht2x_driver,
        .bus = &I2CD4,
        .priv = &aht2x_dev_data,
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
        .name = "bh1750",
        .driver = &bh1750_driver,
        .bus = &I2CD4,
        .priv = &bh1750_dev_data,
        .is_active = false
    },
    {
        .name = "24lc256",
        .driver = &eeprom_24lc256_driver,
        .bus = &I2CD4,
        .priv = &eeprom_24lc256_dev_data,
        .is_active = false
    },
    {
        .name = "w25qxx",
        .driver = &w25qxx_driver,
        .bus = &spi_bus_w25qxx,
        .priv = &w25qxx_dev_data,
        .is_active = false
    },
    /*
    {
        .name = "servo",
        .driver = &servo_driver,
        .bus = NULL,
        .priv = NULL,
        .is_active = false
    },
    */
};


// number of devices on APM
const size_t num_board_devices = sizeof(board_devices) / sizeof(board_devices[0]);


void bsp_init(void) {
  halInit();
  chSysInit();

  // initialize debug UART
  bsp_io_init();

  bsp_printf("\n...Starting...\n\n");

  bsp_printf("booting...");
  for (uint8_t i = 0; i < 3; i++) {
    bsp_printf(".");
    chThdSleepMilliseconds(250);
  }
  bsp_printf("\n");

  // chip info
  bsp_printf("System tick freq: %u Hz\n", CH_CFG_ST_FREQUENCY);
  bsp_printf("DBGMCU->IDCODE: 0x%08lX\r\n", DBGMCU->IDCODE);
  bsp_printf("REV_ID: 0x%X\r\nDEV_ID: 0x%X\n", DBGMCU->IDCODE >> 16,
                                            DBGMCU->IDCODE & 0xFFF);

  // initialize drivers this board supports
  bsp_i2c_init();
  bsp_printf("I2C initialized\n");

  bsp_spi_init();
  bsp_printf("SPI initialized\n");

  init_devices();
  bsp_printf("initialized devices\n");

/*
  // these should be configured more elegantly like how the i2c stuff is configured
  // ADC inputs
  palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG); // PB1, ADC channel 5
  palSetPadMode(GPIOA, 3, PAL_MODE_INPUT_ANALOG); // PA3, ADC channel 15
  palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG); // PC0, ADC channel 10
  palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG); // PC3, ADC channel 13

  // PWM on PE11, timer 1 channel 2?
  palSetPadMode(GPIOE, 11, PAL_MODE_ALTERNATE(1));
*/

  bsp_printf("End of BSP init\n\r\n");
}
