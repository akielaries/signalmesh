#include <math.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/bme280.h"


/* 100KHz timing: keep your timing value if it works on your board */
static const I2CConfig i2c_config = {
  .timingr = 0x00C0EAFF, // 100kHz example
  //.timingr = 0x00D0A3FF,
  //.timingr =   STM32_TIMINGR_PRESC(0x3U) |
  //STM32_TIMINGR_SCLDEL(0x7U) | STM32_TIMINGR_SDADEL(0x0U) |
  //STM32_TIMINGR_SCLH(0x75U)  | STM32_TIMINGR_SCLL(0xB1U),
  //.timingr = 0x10C0ECFF,
  .cr1 = 0,
  .cr2 = 0
};

int main(void) {
  bsp_init();

  bsp_printf("--- Starting INA219 Test ---\r\n");
  bsp_printf("Testing INA219 current/power sensor.\r\n");
  bsp_printf("Press button to stop.\r\n\r\n");
  bsp_printf("PCLK1 = %u Hz\n", STM32_PCLK1);

  // I2CD1
  //palSetPadMode(GPIOB, 8,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);
  //palSetPadMode(GPIOB, 9,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN);

  // I2CD4
  palSetPadMode(GPIOD, 12,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOD, 13,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_PUPDR_PULLUP);


  // reset clock control enable clocks
  rccEnableI2C4(true);

  // start I2C drivers
  i2cStart(&I2CD4, &i2c_config);

  bme280_probe();

  bsp_printf("finished\n");

  while (true) {
    if (palReadLine(LINE_BUTTON) == PAL_HIGH) {
      bsp_printf("INA219 test stopped.\n");
      break;
    }

    chThdSleepMilliseconds(1000);
  }

  return 0;
}

