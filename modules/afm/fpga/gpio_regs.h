#ifndef __CHEBY__GPIO_REGS__H__
#define __CHEBY__GPIO_REGS__H__

#define GPIO_REGS_SIZE 8 /* 0x8 */

/* REG out */
#define GPIO_REGS_OUT 0x0UL

/* REG stat */
#define GPIO_REGS_STAT 0x4UL

#ifndef __ASSEMBLER__
struct gpio_regs {
  /* [0x0]: REG (rw) */
  uint32_t out;

  /* [0x4]: REG (ro) */
  uint32_t stat;
};
#endif /* !__ASSEMBLER__*/

#endif /* __CHEBY__GPIO_REGS__H__ */
