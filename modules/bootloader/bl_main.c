// signalmesh bootloader entry (STM32/APM). runs before the application. it is
// the golden image: immutable recovery core that accepts updates over UART4.
//
// app model: two app images in QSPI - slot 1 = active/newest (update target),
// slot 0 = last-known-good fallback. on update, the new image is written to
// slot 1 and the previously-active image is copied down to slot 0. on boot the
// chosen valid slot is copied into the internal-flash exec region and run.
//
// boot cascade: slot 1 -> slot 0 -> golden (stay here, print uptime, keep the
// UART open). SKELETON - the marked steps are yours to fill in. it uses the
// concrete drivers directly (qspi_memmap, the flash HAL, the UART4 DMA code from
// test_uart4_rx_bench.c).

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

#include "drivers/qspi_memmap.h"

#include "bootloader/protocol.h"
#include "bootloader/frame.h"
#include "bootloader/crc32.h"
#include "bootloader/image.h"
#include "bootloader/memmap.h"

// TODO: reuse the qspi_memmap_config_t used by test_qspi_bench (dual, base
// 0x90000000, 16MB). declare it extern from the bsp or define here.
// extern const qspi_memmap_config_t qspi_cfg;

// TODO: copy a QSPI app slot into the internal-flash exec region (erase +
// program), so the linked-once app can run. return 0 on success.
static int stage_app(enum bl_slot slot) {
  (void)slot;
  return -1;
}

// TODO: jump to the app at BL_SLOT_APP_EXEC (set MSP from word0, VTOR, branch to
// the reset vector at word1).
static void jump_to_app(uint32_t base) {
  (void)base;
}

int main(void) {
  // minimal init - deliberately NOT bsp_init() (that runs the whole device
  // registry: I2C sensors, INA219s, OLED, LCD - seconds of delay). the
  // bootloader wants only clocks, the RTOS, and the debug console; it brings up
  // UART4 and QSPI itself.
  halInit();
  chSysInit();
  bsp_io_init(); // debug console on UART5
  bsp_printf("\n--- signalmesh bootloader ---\r\n");

  // bring up QSPI so the app slots + fpga bitstream are readable as memory.
  // TODO: configure the QUADSPI AF9 pins (PB2/PG6/PD11-13/PE2 - lift from
  // apm/bsp/configs/bsp_spi_config.c) then qspi_memmap_init(&qspi_cfg) and set
  // qspi_ready. until QSPI is memory-mapped, touching 0x90.. bus-faults, so the
  // boot cascade below is skipped and we drop straight to the golden loop.
  int qspi_ready = 0;

  // 1) TODO: update requested? (host HELLO on UART4 within a window / button /
  //    a flag in BL_SLOT_PARAMS). if so, lift the UART4 DMA + handshake from
  //    test_uart4_rx_bench.c and run a session:
  //      MANIFEST APM_H755 -> copy the current active image (slot1) down to
  //        slot0, then write the new image to slot1 (qspi_memmap_program).
  //      MANIFEST FPGA_*    -> write BL_SLOT_FPGA_ACTIVE.
  //      verify crc vs manifest, write a bl_image_header, reply RESULT.

  // 2) boot cascade: newest first, then fallback. needs QSPI (slots live at
  // 0x90..), so it is skipped until qspi_ready is set above.
  if (qspi_ready) {
    bsp_printf("QSPI ready, reading slots...\n");

    static const enum bl_slot order[] = {BL_SLOT_APP_1, BL_SLOT_APP_0};
    for (unsigned i = 0; i < 2U; i++) {
      enum bl_slot slot = order[i];
      if (bl_image_validate((const void *)bl_memmap[slot].base) != 0) {
        continue; // stored image bad, try the next
      }
      if (stage_app(slot) != 0) {
        continue; // copy to exec region failed
      }
      if (bl_image_validate((const void *)bl_memmap[BL_SLOT_APP_EXEC].base) != 0) {
        continue; // exec copy didn't verify
      }
      bsp_printf("booting app from %s\r\n", bl_memmap[slot].name);
      // TODO: load the fpga bitstream (BL_SLOT_FPGA_ACTIVE; golden on failure)
      // and hold the DAC muted until the FPGA asserts DONE + a magic readback.
      jump_to_app(bl_memmap[BL_SLOT_APP_EXEC].base); // no return on success
    }
  } else {
    bsp_printf("QSPI not ready...\n");
  }

  // 3) golden fallback: no app validated. the bootloader IS the golden image -
  // stay resident, print uptime, keep the UART link open for a host to push a
  // new image at any time.
  bsp_printf("no valid app. staying in bootloader\r\n");
  uint32_t uptime_s = 0U;
  for (;;) {
    bsp_printf("bootloader up %lu s\r\n", (unsigned long)uptime_s++);
    chThdSleepMilliseconds(1000);
  }
}
