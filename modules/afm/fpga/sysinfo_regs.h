#ifndef __CHEBY__SYSINFO_REGS__H__
#define __CHEBY__SYSINFO_REGS__H__

#define SYSINFO_REGS_SIZE 20 /* 0x14 */

/* REG magic */
#define SYSINFO_REGS_MAGIC 0x0UL

/* REG mfg_code_A */
#define SYSINFO_REGS_MFG_CODE_A 0x4UL

/* REG mfg_code_B */
#define SYSINFO_REGS_MFG_CODE_B 0x8UL

/* REG version */
#define SYSINFO_REGS_VERSION 0xcUL
#define SYSINFO_REGS_VERSION_PATCH_MASK 0xffUL
#define SYSINFO_REGS_VERSION_PATCH_SHIFT 0
#define SYSINFO_REGS_VERSION_MINOR_MASK 0xff00UL
#define SYSINFO_REGS_VERSION_MINOR_SHIFT 8
#define SYSINFO_REGS_VERSION_MAJOR_MASK 0xffff0000UL
#define SYSINFO_REGS_VERSION_MAJOR_SHIFT 16

/* REG cheby_version */
#define SYSINFO_REGS_CHEBY_VERSION 0x10UL
#define SYSINFO_REGS_CHEBY_VERSION_PATCH_MASK 0xffUL
#define SYSINFO_REGS_CHEBY_VERSION_PATCH_SHIFT 0
#define SYSINFO_REGS_CHEBY_VERSION_MINOR_MASK 0xff00UL
#define SYSINFO_REGS_CHEBY_VERSION_MINOR_SHIFT 8
#define SYSINFO_REGS_CHEBY_VERSION_MAJOR_MASK 0xffff0000UL
#define SYSINFO_REGS_CHEBY_VERSION_MAJOR_SHIFT 16

#ifndef __ASSEMBLER__
struct sysinfo_regs {
  /* [0x0]: REG (ro) */
  uint32_t magic;

  /* [0x4]: REG (ro) */
  uint32_t mfg_code_A;

  /* [0x8]: REG (ro) */
  uint32_t mfg_code_B;

  /* [0xc]: REG (ro) */
  uint32_t version;

  /* [0x10]: REG (ro) */
  uint32_t cheby_version;
};
#endif /* !__ASSEMBLER__*/

#endif /* __CHEBY__SYSINFO_REGS__H__ */
