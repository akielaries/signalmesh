#include "hal.h"
#include "ch.h"
#include "bsp/configs/bsp_i2c_config.h"


// Using timing from CubeH7 example (0x00901954 for 400kHz at 100MHz APB1)
const I2CConfig bsp_i2c_config = {
  //.timingr = 0x00901954, // 400kHz w/ 100mHz timer
  .timingr = 0x00C0EAFF, // 100kHz w/ 100mHz timer?
  .cr1 = 0,
  .cr2 = 0
};

// Use I2C4 as the default I2C driver
I2CDriver *const bsp_i2c_driver = &I2CD4;


void bsp_i2c_init(void) {
    // I2C4
    palSetPadMode(GPIOD,
                  12,
                  PAL_MODE_ALTERNATE(4) |
                  PAL_STM32_OTYPE_OPENDRAIN |
                  PAL_STM32_PUPDR_PULLUP);
    palSetPadMode(GPIOD,
                  13,
                  PAL_MODE_ALTERNATE(4) |
                  PAL_STM32_OTYPE_OPENDRAIN |
                  PAL_STM32_PUPDR_PULLUP);

    // reset clock control enable clocks
    rccEnableI2C4(true);

    // start I2C drivers
    i2cStart(bsp_i2c_driver, &bsp_i2c_config);
}
