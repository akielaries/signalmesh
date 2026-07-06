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
#include "drivers/w25qxx.h"

#include "bootloader/protocol.h"
#include "bootloader/frame.h"
#include "bootloader/crc32.h"
#include "bootloader/image.h"
#include "bootloader/memmap.h"

#include "version_gen.h" // generated: FW_VERSION_STRING etc.

#include "bl_update.h"

// QSPI config - same as test_qspi_bench: 16MB W25Q128, dual lines, memory-mapped
// at 0x90000000. the clock prescaler is a build-time mcuconf setting (from apm).
static const WSPIConfig wspi_cfg = {
  .end_cb   = NULL,
  .error_cb = NULL,
  .dcr      = STM32_DCR_FSIZE(23U) | STM32_DCR_CSHT(1U), // FSIZE = log2(16MB)-1
};

static const qspi_memmap_config_t qspi_cfg = {
  .wspi       = &WSPID1,
  .wspi_cfg   = &wspi_cfg,
  .base       = 0x90000000UL,
  .size_bytes = W25Q128_SIZE_BYTES,
  .lines      = W25QXX_QSPI_DUAL,
};

// bring up QSPI: configure the QUADSPI pins and init the device (indirect mode).
// returns 1 on success. the cascade enters memory-mapped mode to read the slots;
// the update path uses indirect erase/program.
static int qspi_bringup(void) {
  // QUADSPI pins, lifted from apm/bsp/configs/bsp_spi_config.c (AF9 except NCS)
  palSetPadMode(GPIOB, 2,  PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST); // CLK
  palSetPadMode(GPIOG, 6,  PAL_MODE_ALTERNATE(10) | PAL_STM32_OSPEED_LOWEST); // NCS (BK1)
  palSetPadMode(GPIOD, 11, PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST); // IO0
  palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST); // IO1
  palSetPadMode(GPIOE, 2,  PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST |
                           PAL_STM32_PUPDR_PULLUP); // IO2/WP
  palSetPadMode(GPIOD, 13, PAL_MODE_ALTERNATE(9)  | PAL_STM32_OSPEED_LOWEST |
                           PAL_STM32_PUPDR_PULLUP); // IO3/HOLD

  return qspi_memmap_init(&qspi_cfg) ? 1 : 0;
}

// hand off to the app at `app_base` (the app's vector table): stop the RTOS
// tick, flush caches (the app runs XIP from QSPI + reads its rodata), point VTOR
// at it, load its stack pointer, and branch to its reset vector. never returns.
static void jump_to_app(uint32_t app_base) {
  const uint32_t *v = (const uint32_t *)app_base;
  uint32_t sp = v[0];
  uint32_t reset = v[1];

  __disable_irq();
  SysTick->CTRL = 0U;                 // stop the ChibiOS tick
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk; // clear any pending tick

  SCB_CleanInvalidateDCache();        // app reads freshly-written QSPI
  SCB_InvalidateICache();             // and executes from it (XIP)

  SCB->VTOR = app_base;
  __DSB();
  __set_MSP(sp);
  __DSB();
  __ISB();
  ((void (*)(void))reset)();
  for (;;) {
  }
}

int main(void) {
  // minimal init - deliberately NOT bsp_init() (that runs the whole device
  // registry: I2C sensors, INA219s, OLED, LCD - seconds of delay). the
  // bootloader wants only clocks, the RTOS, and the debug console; it brings up
  // UART4 and QSPI itself.
  halInit();
  chSysInit();
  bsp_io_init(); // debug console on UART5
  bsp_printf("\n--- %s ---\r\n", FW_VERSION_STRING); // e.g. bootloader v1.0.0-7e98556-dirty

  // bring up QSPI (pins + device init). until this succeeds, touching 0x90..
  // bus-faults, so the boot cascade is gated on it.
  int qspi_ready = qspi_bringup();
  bsp_printf("QSPI: %s\r\n",
             qspi_ready ? "up (W25Q128 dual, memmap @ 0x90000000)" : "init FAILED");

  // 1) update mode: give a host a short window to connect on UART4 and push an
  //    image. bl_update_run receives it (MANIFEST/DATA/DONE), verifies, and
  //    writes the target QSPI slot; the boot cascade below then boots it.
  //    runs in indirect mode (qspi bringup left it there); the cascade enables
  //    memory-mapped mode afterwards.
  //    TODO: slot rotation - copy the current active image (slot1) down to slot0
  //    before overwriting slot1, so slot0 stays the last-known-good.
  if (qspi_ready) {
    bl_update_run(&qspi_cfg, 5000U); // 5s window to catch a host (tune down later)
  }

  // 2) boot cascade: newest first, then fallback. needs QSPI (slots live at
  // 0x90..), so it is skipped until qspi_ready is set above.
  if (qspi_ready) {
    // enter memory-mapped mode so the slots at 0x90.. are readable by pointer
    qspi_memmap_enable(&qspi_cfg);
    bsp_printf("QSPI ready, reading slots...\n");

    static const enum bl_slot order[] = {BL_SLOT_APP_1, BL_SLOT_APP_0};
    for (unsigned i = 0; i < 2U; i++) {
      enum bl_slot slot = order[i];
      if (bl_image_validate((const void *)bl_memmap[slot].base) != 0) {
        continue; // stored image bad, try the next
      }
      // XIP: run the app in place from its QSPI slot (no copy). the image sits
      // at BL_IMAGE_OFFSET into the slot (its vector table).
      uint32_t app = bl_memmap[slot].base + BL_IMAGE_OFFSET;
      bsp_printf("booting app from %s (XIP @ 0x%08lX)\r\n", bl_memmap[slot].name,
                 (unsigned long)app);
      // TODO: load the fpga bitstream (BL_SLOT_FPGA_ACTIVE; golden on failure)
      // and hold the DAC muted until the FPGA asserts DONE + a magic readback.
      jump_to_app(app); // no return on success
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
