// minimal bare-metal demo app for the bootloader stage-to-flash path. the
// bootloader copies this image from its QSPI slot into the internal-flash exec
// region (0x08100000) and runs it from there (no ChibiOS, no bsp). prints a
// banner on UART5 - which the bootloader already configured and left running -
// and blinks the green LED, so you visibly see the hand off. no .data/.bss, so
// no startup init is needed: the vector table's reset entry jumps straight to
// Reset_Handler.

#include <stdint.h>

// UART5 registers (debug console; the bootloader set the baud, we just write)
#define UART5_ISR (*(volatile uint32_t *)(0x40005000UL + 0x1CU))
#define UART5_TDR (*(volatile uint32_t *)(0x40005000UL + 0x28U))
#define USART_TXE (1U << 7) // TXFNF

// GPIOB (green LED = PB0 on the Nucleo-H755ZI); clock already on from halInit
#define GPIOB_MODER (*(volatile uint32_t *)(0x58020400UL + 0x00U))
#define GPIOB_ODR   (*(volatile uint32_t *)(0x58020400UL + 0x14U))
#define LED_PIN 0U

extern uint32_t _estack; // top of RAM, provided by the linker

static void put5(char c) {
  while ((UART5_ISR & USART_TXE) == 0U) {
  }
  UART5_TDR = (uint32_t)(uint8_t)c;
}

static void print5(const char *s) {
  while (*s != '\0') {
    put5(*s++);
  }
}

__attribute__((noreturn)) void Reset_Handler(void) {
  GPIOB_MODER = (GPIOB_MODER & ~(3U << (LED_PIN * 2U))) | (1U << (LED_PIN * 2U));

  print5("\r\n>>> demo app v1.0.0 booted (from internal flash) <<<\r\n");

  for (;;) {
    GPIOB_ODR ^= (1U << LED_PIN);
    print5("app tick\r\n");
    for (volatile uint32_t d = 0; d < 4000000U; d++) {
    }
  }
}

// minimal vector table: initial SP + reset handler. the bootloader points VTOR
// here, loads word0 into MSP, and branches to word1 (Reset_Handler).
__attribute__((section(".vectors"), used))
const uint32_t vectors[] = {
  (uint32_t)&_estack,
  (uint32_t)Reset_Handler,
};
