#ifndef __CHEBY__CORE__H__
#define __CHEBY__CORE__H__

#define CORE_SIZE 8 /* 0x8 */

/* REG magic */
#define CORE_MAGIC 0x0UL

/* REG fpga_id */
#define CORE_FPGA_ID 0x2UL

/* REG scratch */
#define CORE_SCRATCH 0x4UL

/* REG version */
#define CORE_VERSION 0x6UL

#ifndef __ASSEMBLER__
struct core {
  /* [0x0]: REG (ro) */
  uint16_t magic;

  /* [0x2]: REG (ro) */
  uint16_t fpga_id;

  /* [0x4]: REG (rw) */
  uint16_t scratch;

  /* [0x6]: REG (ro) */
  uint16_t version;
};
#endif /* !__ASSEMBLER__*/

#endif /* __CHEBY__CORE__H__ */
