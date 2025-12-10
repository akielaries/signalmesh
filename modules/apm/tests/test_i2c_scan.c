#include <string.h>
#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"

/* 100KHz timing */
static const I2CConfig i2c_config = {
  .timingr = 0x00C0EAFF,
  //.timingr =   STM32_TIMINGR_PRESC(0x3U) |
  //STM32_TIMINGR_SCLDEL(0x7U) | STM32_TIMINGR_SDADEL(0x0U) |
  //STM32_TIMINGR_SCLH(0x75U)  | STM32_TIMINGR_SCLL(0xB1U),
  .cr1 = 0,
  .cr2 = 0
};

static void i2c_scan(I2CDriver *i2c_driver) {
    bsp_printf("Scanning I2C bus...\r\n");
    uint8_t dummy = 0;
    int count = 0;

    for (uint8_t addr = 1; addr < 128; addr++) {
        msg_t status = i2cMasterTransmitTimeout(
            i2c_driver,
            addr,
            &dummy, 0,          // 0-byte write
            NULL, 0,
            TIME_MS2I(10)
        );

        if (status == MSG_OK) {
            bsp_printf("  Found device at 0x%02X\r\n", addr);
            count++;
        } else {
            i2cflags_t err = i2cGetErrors(i2c_driver);
            if (err != I2C_NO_ERROR) {
                bsp_printf("  Addr 0x%02X: err=0x%02X\r\n", addr, err);
            }
        }

        chThdSleepMilliseconds(3);
    }

    if (count == 0)
        bsp_printf("No I2C devices found.\r\n");
    else
        bsp_printf("Total: %d devices.\r\n", count);
}

int main(void) {
  bsp_init();

  bsp_printf("--- Starting I2C Scan Test ---\r\n");
    palSetPadMode(GPIOB, 8,  PAL_MODE_ALTERNATE(4) |
                           PAL_STM32_OTYPE_OPENDRAIN |
                           PAL_STM32_PUPDR_PULLUP);

    palSetPadMode(GPIOB, 9,  PAL_MODE_ALTERNATE(4) |
                           PAL_STM32_OTYPE_OPENDRAIN |
                           PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOD, 13, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP);

    rccEnableI2C4(true);

    i2cStart(&I2CD4, &i2c_config);
    chThdSleepMilliseconds(50);

    i2c_scan(&I2CD4);

    while (true) {
        chThdSleepMilliseconds(1000);
    }

}
