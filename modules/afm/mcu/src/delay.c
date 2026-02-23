#include "delay.h"
#include "kernel.h"
#include "GOWIN_M1.h"


/* Definitions ---------------------------------------------------------------*/

/* Delay macros */
#define STEP_DELAY_MS 50


/* Delay variable */
static __IO uint32_t fac_us;
static __IO uint32_t fac_ms;


/**
 * @brief  Initialize delay function
 * @param  None
 * @retval None
 */
void delay_init(void) {
  /* Configure systick */
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

  // uint32_t clock = 1000000U;
  // uint32_t clock = 27000000U;
  // uint32_t clock = 50000000U;

  // fac_us = SystemCoreClock / (clock);
  fac_us = SystemCoreClock / 1000000;
  fac_ms = fac_us * (1000U);
  /*
   * Configure the SysTick timer to generate an interrupt every 1 millisecond.
   *
   * With a SystemCoreClock of 50MHz, we need to set the reload value to
   * (50,000,000 / 1,000) - 1 = 49,999.
   */
  SysTick_Config(SystemCoreClock / 1000);
}

/**
 * @brief  Inserts a delay time.
 * @param  nus: Specifies the delay time length, in microsecond.
 * @retval None
 */
void delay_us(uint32_t nus) {
  uint32_t temp = 0;

  SysTick->LOAD = (uint32_t)(nus * fac_us);
  SysTick->VAL  = 0x00;
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

  do {
    temp = SysTick->CTRL;
  } while ((temp & 0x01) && !(temp & (1 << 16)));

  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  SysTick->VAL = 0x00;
}

/**
 * @brief  Inserts a delay time.
 * @param  nms: Specifies the delay time length, in milliseconds.
 * @retval None
 */
void delay_ms(uint16_t nms) {
  uint32_t temp = 0;

  while (nms) {
    if (nms > STEP_DELAY_MS) {
      SysTick->LOAD = (uint32_t)(STEP_DELAY_MS * fac_ms);
      nms -= STEP_DELAY_MS;
    } else {
      SysTick->LOAD = (uint32_t)(nms * fac_ms);
      nms           = 0;
    }

    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    do {
      temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0x00;
  }
}

/**
 * @brief  Inserts a delay time.
 * @param  sec: Specifies the delay time, in seconds.
 * @retval None
 */
void delay_sec(uint16_t sec) {
  uint16_t index;

  for (index = 0; index < sec; index++) {
    delay_ms(500);
    delay_ms(500);
  }
}
