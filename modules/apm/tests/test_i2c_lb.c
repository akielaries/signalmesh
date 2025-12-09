#include <string.h>
#include <math.h>

#include "ch.h"
#include "hal.h"
#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"
#include "bsp/configs/bsp_uart_config.h"
#include "bsp/include/bsp_defs.h"
#include "drivers/ina219.h"

#define MASTER_SUCCESS_LED APM_BSP_OK
#define MASTER_ERROR_LED   APM_BSP_ERR
#define SLAVE_ACTIVITY_LED APM_BSP_WARN

/*
 * NOTE: ChibiOS I2C APIs expect a 7-bit address. If you had an 8-bit EEPROM
 * address like 0xA0 (write) / 0xA1 (read), use the 7-bit base (0x50).
 *
 * Change this to the exact 7-bit slave address you want.
 */
#define I2C_LOOPBACK_SLAVE_ADDR 0x18

/* 100KHz timing: keep your timing value if it works on your board */
static const I2CConfig i2c_config = {
  //.timingr = 0x00C0EAFF, // 100kHz example
  .timingr =   STM32_TIMINGR_PRESC(0x3U) |
  STM32_TIMINGR_SCLDEL(0x7U) | STM32_TIMINGR_SDADEL(0x0U) |
  STM32_TIMINGR_SCLH(0x75U)  | STM32_TIMINGR_SCLL(0xB1U),
  .cr1 = 0,
  .cr2 = 0
};

/* Buffer sizes */
#define LB_BUF_SIZE  32

/* Buffers (aligned / placed normally). The ChibiOS sample uses special macros
 * for cache alignment/placement. We'll use the same idea but keep it portable. */
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
static uint8_t master_tx[LB_BUF_SIZE];
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
static uint8_t master_rx[LB_BUF_SIZE];

#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
CC_SECTION(".ram4")
static uint8_t slave_tx[LB_BUF_SIZE];
#if CACHE_LINE_SIZE > 0
CC_ALIGN_DATA(CACHE_LINE_SIZE)
#endif
CC_SECTION(".ram4")
static uint8_t slave_rx[LB_BUF_SIZE];

/* Cache helpers: if your project already defines cacheBufferFlush/invalidate
 * these macros will resolve to them. Otherwise they become no-ops. On STM32H7
 * projects with D-Cache enabled you should map these to the CMSIS functions
 * (SCB_CleanDCache_by_Addr / SCB_InvalidateDCache_by_Addr) or use non-cacheable memory. */
#ifndef cacheBufferFlush
#define cacheBufferFlush(buf, len)    /* implement if needed */
#endif
#ifndef cacheBufferInvalidate
#define cacheBufferInvalidate(buf, len) /* implement if needed */
#endif

void handle_i2c_err(int err) {
  if (err & I2C_BUS_ERROR) {
    bsp_printf("BUS ERROR!\n");
  } else if (err & I2C_ARBITRATION_LOST) {
    bsp_printf("ARBITRATION LOST!\n");
  } else if (err & I2C_ACK_FAILURE) {
    bsp_printf("I2C ACK FAILURE!\n");
  } else if (err & I2C_OVERRUN) {
    bsp_printf("I2C OVERRUN!\n");
  } else {
    bsp_printf("some other failure!\n");
  }
}

/* Master thread: does Transmit-then-Receive operation similar to chibios test */
static THD_WORKING_AREA(waMasterThread, 512);
static THD_FUNCTION(MasterThread, arg) {
  (void)arg;
  chRegSetThreadName("I2C Master");

  uint8_t counter = 0;
  while (true) {
    /* prepare master_tx: one byte payload that increments */
    memset(master_tx, 0, LB_BUF_SIZE);
    master_tx[0] = counter;

    /* flush cache for master_tx before DMA/Master transmit */
    cacheBufferFlush(master_tx, 1);

    /* Do a transmit-then-receive operation (1 byte each here). */
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1,
                                         I2C_LOOPBACK_SLAVE_ADDR,
                                         master_tx, 1,
                                         master_rx, 1,
                                         TIME_MS2I(200)); /* 200ms timeout */

    if (msg == MSG_OK) {
      /* invalidate cache for master_rx after DMA completes */
      cacheBufferInvalidate(master_rx, 1);

      if (master_rx[0] == counter) {
        palToggleLine(MASTER_SUCCESS_LED);
      } else {
        bsp_printf("master: rx mismatch (sent 0x%02X got 0x%02X)\n",
                   counter, master_rx[0]);
        palToggleLine(MASTER_ERROR_LED);
      }
    } else {
      bsp_printf("master err: %d\n", msg);
      int ret = i2cGetErrors(&I2CD1);
      bsp_printf("i2c errs: %d\n", ret);
      handle_i2c_err(ret);
      palToggleLine(MASTER_ERROR_LED);
    }

    counter++;
    chThdSleepMilliseconds(250);
  }
}

/* Slave thread: waits for master transfer and, if master expects reply, sends it */
static THD_WORKING_AREA(waSlaveThread, 512);
static THD_FUNCTION(SlaveThread, arg) {
  (void)arg;
  chRegSetThreadName("I2C Slave");

  memset(slave_rx, 0, LB_BUF_SIZE);
  /* prepare slave_tx with a base message (optional) */
  memset(slave_tx, 0, LB_BUF_SIZE);
  memcpy(slave_tx, "Loopback Slave reply", 20);

  while (true) {
    /* Wait for a master transmission into slave_rx */
    msg_t rx_msg = i2cSlaveReceiveTimeout(&I2CD4, slave_rx, 1, TIME_INFINITE);

    if (rx_msg == MSG_OK) {
      /* invalidate the buffer to ensure CPU sees what DMA wrote */
      cacheBufferInvalidate(slave_rx, 1);

      /* the received byte we will echo back */
      uint8_t rx_b = slave_rx[0];
      slave_tx[0] = rx_b;

      palToggleLine(SLAVE_ACTIVITY_LED);

      /* flush slave_tx so DMA sees CPU changes */
      cacheBufferFlush(slave_tx, 1);

      /* If the master will read back (i2cSlaveIsAnswerRequired) then respond */
      if (i2cSlaveIsAnswerRequired(&I2CD4)) {
        msg_t tx_msg = i2cSlaveTransmitTimeout(&I2CD4, slave_tx, 1, TIME_INFINITE);
        if (tx_msg != MSG_OK) {
          bsp_printf("slave tx err: %d\n", tx_msg);
          int ret = i2cGetErrors(&I2CD4);
          bsp_printf("i2c errs: %d\n", ret);
          palToggleLine(MASTER_ERROR_LED);
        }
      }

      /* clear the rx buffer for next time (optional) */
      memset(slave_rx, 0, 1);
    } else {
      /* probably never happen since TIME_INFINITE, but handle it */
      bsp_printf("slave receive returned: %d\n", rx_msg);
    }
  }
}

int main(void) {
  // seems this has no impact?
  //rccResetAHB4(STM32_GPIO_EN_MASK);
  //rccEnableAHB4(STM32_GPIO_EN_MASK, true);
  bsp_init();

  bsp_printf("--- Starting I2C Loopback Test ---\r\n");
  bsp_printf("I2C1 (Master) <--> I2C4 (Slave @ 0x%02X)\r\n", I2C_LOOPBACK_SLAVE_ADDR);
  bsp_printf("Green LED: Master OK | Red LED: Error | Yellow LED: Slave RX\r\n");

  palSetPadMode(GPIOB, 8,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP); /* I2C1 SCL (example) */
  palSetPadMode(GPIOB, 9,  PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP); /* I2C1 SDA */

  /* slave I2C4 SCL on PD12, SDA on PD13 */
  palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOD, 13, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN |
                          PAL_STM32_PUPDR_PULLUP);

  // reset clock control enable clocks
  rccEnableI2C1(true);
  rccEnableI2C4(true);

  // start I2C drivers
  i2cStart(&I2CD1, &i2c_config); /* Master */
  i2cStart(&I2CD4, &i2c_config); /* Slave */

  chThdSleepMilliseconds(50);

  // bind the slave address so the peripheral will ACK master's address */
  i2cSlaveMatchAddress(&I2CD4, I2C_LOOPBACK_SLAVE_ADDR);

  bsp_printf("I2C Peripherals started and configured.\r\n");

  chThdCreateStatic(waSlaveThread, sizeof(waSlaveThread), NORMALPRIO, SlaveThread, NULL);
  chThdCreateStatic(waMasterThread, sizeof(waMasterThread), NORMALPRIO, MasterThread, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
  return 0;
}

