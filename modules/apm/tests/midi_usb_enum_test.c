// USB host bring-up: enumerate a USB-MIDI keyboard on OTG_FS and dump its
// descriptors. milestone 1 - no MIDI parsing yet, just prove enumeration.
//
// chain: keyboard -> OTG_FS (PA11/PA12) -> ChibiOS-Contrib USBH -> debug uart
// VBUS is supplied externally (powered hub / PSU), grounds common.

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"

extern SerialDriver *const bsp_debug_uart_driver;

// sink for the USBH debug subsystem (name matches USBH_DEBUG_OUTPUT_CALLBACK).
// the stack's debug thread drains its ring buffer into here.
void usbh_debug_output(const uint8_t *buff, size_t len) {
  sdWrite(bsp_debug_uart_driver, buff, len);
  sdWrite(bsp_debug_uart_driver, (const uint8_t *)"\r\n", 2);
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- midi_usb_test: USB host enumeration on OTG_FS ---\r\n");

  // OTG_FS data pins. VBUS is external so no power switch / sensing here.
  palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(10) | PAL_STM32_OSPEED_HIGHEST);  // OTG_FS_DM
  palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(10) | PAL_STM32_OSPEED_HIGHEST);  // OTG_FS_DP

  usbhStart(&USBHD2);   // OTG2 = USB2_OTG_FS = PA11/PA12 on H7
  bsp_printf("USBH started, plug in the keyboard...\r\n");

  // one-time health dump: is the OTG_FS core clocked, clocked at 48 MHz, and
  // is the USB transceiver supply ready?
  bsp_printf("GSNPSID=0x%08lX GINTSTS=0x%08lX GCCFG=0x%08lX\r\n",
             (unsigned long)*(volatile uint32_t *)0x40080040UL,
             (unsigned long)*(volatile uint32_t *)0x40080014UL,
             (unsigned long)*(volatile uint32_t *)0x40080038UL);
  bsp_printf("AHB1ENR=0x%08lX RCC_CR=0x%08lX D2CCIP2R=0x%08lX PWR_CR3=0x%08lX\r\n",
             (unsigned long)RCC->AHB1ENR, (unsigned long)RCC->CR,
             (unsigned long)RCC->D2CCIP2R, (unsigned long)PWR->CR3);
  // PA11/PA12 should read MODER=10b (AF) and AFRH nibble=0xA (AF10) for OTG_FS.
  // PA11: MODER[23:22], AFRH[15:12]   PA12: MODER[25:24], AFRH[19:16]
  bsp_printf("GPIOA_MODER=0x%08lX GPIOA_AFRH=0x%08lX GPIOA_OSPEEDR=0x%08lX\r\n",
             (unsigned long)GPIOA->MODER, (unsigned long)GPIOA->AFRH,
             (unsigned long)GPIOA->OSPEEDR);
  // host-PHY state: GUSBCFG (PHYSEL b6 / FHMOD b29), GRSTCTL (AHBIDL b31 / CSRST b0),
  // GOTGCTL (session valid), HCFG (FSLSPCS[1:0] must be 01 = 48MHz for FS host)
  bsp_printf("GOTGCTL=0x%08lX GUSBCFG=0x%08lX GRSTCTL=0x%08lX HCFG=0x%08lX\r\n",
             (unsigned long)*(volatile uint32_t *)0x40080000UL,
             (unsigned long)*(volatile uint32_t *)0x4008000CUL,
             (unsigned long)*(volatile uint32_t *)0x40080010UL,
             (unsigned long)*(volatile uint32_t *)0x40080400UL);

  // the LLD's HCFG.FSLSPCS write read back as 0 - force 48MHz FS host clock
  // and confirm whether the field latches (it only will if the PHY clock runs)
  volatile uint32_t *const otg_fs_hcfg = (volatile uint32_t *)0x40080400UL;
  *otg_fs_hcfg = (*otg_fs_hcfg & ~0x3u) | 0x1u;
  bsp_printf("HCFG forced -> 0x%08lX (FSLSPCS=%lu)\r\n",
             (unsigned long)*otg_fs_hcfg, (unsigned long)(*otg_fs_hcfg & 3u));

  // VBUS sensing is off (VBDEN=0); on H7 the host won't detect a connect unless
  // it believes the session is valid. force VBUS-valid + A-session-valid.
  volatile uint32_t *const otg_gotgctl = (volatile uint32_t *)0x40080000UL;
  *otg_gotgctl |= (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5);
  bsp_printf("GOTGCTL forced -> 0x%08lX\r\n", (unsigned long)*otg_gotgctl);

  // OTG_FS host port register: PCSTS (bit0) goes high when the PHY detects a
  // device pull-up - a hardware-level probe independent of enumeration/debug.
  volatile uint32_t *const otg_fs_hprt = (volatile uint32_t *)(0x40080000UL + 0x440UL);

  uint32_t tick = 0;
  for (;;) {
    usbhMainLoop(&USBHD2);
    chThdSleepMilliseconds(100);
    if (++tick >= 10) {                 // ~1 Hz
      tick = 0;
      // re-assert FS PHY clock select now that CMOD=1 - the LLD set it during
      // the device->host transition (CMOD=0) when host regs aren't writable
      *otg_fs_hcfg = (*otg_fs_hcfg & ~0x3u) | 0x1u;
      uint32_t hprt = *otg_fs_hprt;
      uint32_t gintsts = *(volatile uint32_t *)0x40080014UL;  // CMOD = bit0 (1=host)
      bsp_printf("HCFG=0x%08lX HPRT=0x%08lX PCSTS=%lu PSPD=%lu CMOD=%lu\r\n",
                 (unsigned long)*otg_fs_hcfg,
                 (unsigned long)hprt,
                 (unsigned long)(hprt & 1u),
                 (unsigned long)((hprt >> 17) & 3u),
                 (unsigned long)(gintsts & 1u));
    }
  }
}
