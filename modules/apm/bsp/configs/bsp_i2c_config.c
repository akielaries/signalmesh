#include "bsp/configs/bsp_i2c_config.h"
#define I2C_TIMINGR_SCLL_Pos (0U)
#define I2C_TIMINGR_SCLL_Msk (0xFFUL << I2C_TIMINGR_SCLL_Pos) /*!< 0x000000FF  \
                                                               */
#define I2C_TIMINGR_SCLL                                                       \
  I2C_TIMINGR_SCLL_Msk /*!< SCL low period (master mode) */
#define I2C_TIMINGR_SCLH_Pos (8U)
#define I2C_TIMINGR_SCLH_Msk (0xFFUL << I2C_TIMINGR_SCLH_Pos) /*!< 0x0000FF00  \
                                                               */
#define I2C_TIMINGR_SCLH                                                       \
  I2C_TIMINGR_SCLH_Msk /*!< SCL high period (master mode) */
#define I2C_TIMINGR_SDADEL_Pos (16U)
#define I2C_TIMINGR_SDADEL_Msk                                                 \
  (0xFUL << I2C_TIMINGR_SDADEL_Pos)                   /*!< 0x000F0000 */
#define I2C_TIMINGR_SDADEL     I2C_TIMINGR_SDADEL_Msk /*!< Data hold time */
#define I2C_TIMINGR_SCLDEL_Pos (20U)
#define I2C_TIMINGR_SCLDEL_Msk                                                 \
  (0xFUL << I2C_TIMINGR_SCLDEL_Pos)                  /*!< 0x00F00000 */
#define I2C_TIMINGR_SCLDEL    I2C_TIMINGR_SCLDEL_Msk /*!< Data setup time */
#define I2C_TIMINGR_PRESC_Pos (28U)
#define I2C_TIMINGR_PRESC_Msk                                                  \
  (0xFUL << I2C_TIMINGR_PRESC_Pos)              /*!< 0xF0000000 */
#define I2C_TIMINGR_PRESC I2C_TIMINGR_PRESC_Msk /*!< Timings prescaler */


// I2C configuration based on STM32CubeH7 example
// Using timing from CubeH7 example (0x00901954 for 400kHz at 100MHz APB1)
const I2CConfig bsp_i2c_config = {
  //.timingr = 0x00901954, // CubeH7 timing for i2C2
  .timingr = 0x00C0EAFF,  // 100KHz from 50MHz PCLK
  .cr1 = 0,
  .cr2 = 0
};

// Use I2C2 as the default I2C driver
I2CDriver *const bsp_i2c_driver = &I2CD4;
