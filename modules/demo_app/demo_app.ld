/* demo app linker: runs XIP from QSPI slot 1's exec address
   (BL_SLOT_APP_1 base 0x90100000 + BL_IMAGE_OFFSET 0x200 = 0x90100200).
   .text/.rodata live in the QSPI-mapped window; stack lives in AXI SRAM. */

ENTRY(Reset_Handler)

MEMORY {
  QSPI (rx)  : ORIGIN = 0x90100200, LENGTH = 256K
  RAM  (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
  .vectors : { KEEP(*(.vectors)) } > QSPI
  .text    : { *(.text*) *(.rodata*) } > QSPI

  /* no initialized globals, so no .data copy; .bss is unused but kept for safety */
  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > RAM
}
