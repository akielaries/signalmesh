/* demo app linker: the app is stored in a QSPI slot but STAGED by the bootloader
   into the internal-flash exec region (BL_SLOT_APP_EXEC = 0x08100000, bank 2)
   and run from there at full core speed. so it links for 0x08100000, not for any
   QSPI address - the bootloader copies the image to this fixed exec base. */

ENTRY(Reset_Handler)

MEMORY {
  FLASH (rx)  : ORIGIN = 0x08100000, LENGTH = 1024K   /* internal flash bank 2 */
  RAM   (rwx) : ORIGIN = 0x24000000, LENGTH = 512K    /* AXI SRAM */
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
  .vectors : { KEEP(*(.vectors)) } > FLASH
  .text    : { *(.text*) *(.rodata*) } > FLASH

  /* no initialized globals, so no .data copy; .bss is unused but kept for safety */
  .bss (NOLOAD) : { *(.bss*) *(COMMON) } > RAM
}
