// STM32H7 internal-flash erase/program. see drivers/stm32h7_flash.h.
//
// sequences follow the H7 reference manual: unlock the bank (KEYR), wait for the
// queue to drain (QW/BSY), then either sector-erase (SER + SNB + START) or
// program a 256-bit flash word (PG, then eight 32-bit stores). errors surface in
// SRx and are cleared by writing them back to CCRx.

#include "hal.h" // CMSIS device header: FLASH, SCB, __DSB
#include <string.h>

#include "drivers/stm32h7_flash.h"

#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

#define FLASH_BANK1_BASE 0x08000000U
#define FLASH_BANK2_BASE 0x08100000U

#define FLASH_ERR_MASK                                                        \
  (FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_STRBERR | FLASH_SR_INCERR |   \
   FLASH_SR_OPERR)

// per-bank register handle so the erase/program logic is bank-agnostic
typedef struct {
  volatile uint32_t *keyr;
  volatile uint32_t *cr;
  volatile uint32_t *sr;
  volatile uint32_t *ccr;
  uint32_t base;
} flash_bank;

static void bank_for(uint32_t addr, flash_bank *b) {
  if (addr >= FLASH_BANK2_BASE) {
    b->keyr = &FLASH->KEYR2;
    b->cr = &FLASH->CR2;
    b->sr = &FLASH->SR2;
    b->ccr = &FLASH->CCR2;
    b->base = FLASH_BANK2_BASE;
  } else {
    b->keyr = &FLASH->KEYR1;
    b->cr = &FLASH->CR1;
    b->sr = &FLASH->SR1;
    b->ccr = &FLASH->CCR1;
    b->base = FLASH_BANK1_BASE;
  }
}

static void bank_unlock(flash_bank *b) {
  if ((*b->cr & FLASH_CR_LOCK) != 0U) {
    *b->keyr = FLASH_KEY1;
    *b->keyr = FLASH_KEY2;
  }
}

static void bank_lock(flash_bank *b) {
  *b->cr |= FLASH_CR_LOCK;
}

// wait for any pending erase/program to finish, then latch+clear errors
static int bank_wait(flash_bank *b) {
  while ((*b->sr & (FLASH_SR_BSY | FLASH_SR_QW)) != 0U) {
  }
  if ((*b->sr & FLASH_ERR_MASK) != 0U) {
    *b->ccr = FLASH_ERR_MASK;
    return -1;
  }
  return 0;
}

int stm32h7_flash_erase(uint32_t addr, uint32_t len) {
  if (len == 0U) {
    return 0;
  }
  flash_bank b;
  bank_for(addr, &b);
  bank_unlock(&b);

  uint32_t first = addr & ~(STM32H7_FLASH_SECTOR_SIZE - 1U);
  uint32_t end = addr + len;
  int rc = 0;
  for (uint32_t a = first; a < end; a += STM32H7_FLASH_SECTOR_SIZE) {
    if (bank_wait(&b) != 0) {
      rc = -1;
      break;
    }
    uint32_t snb = (a - b.base) / STM32H7_FLASH_SECTOR_SIZE; // sector 0..7
    uint32_t cr = *b.cr & ~(FLASH_CR_SNB | FLASH_CR_PSIZE);
    cr |= FLASH_CR_SER | (snb << FLASH_CR_SNB_Pos);
    *b.cr = cr;
    *b.cr |= FLASH_CR_START;
    if (bank_wait(&b) != 0) {
      rc = -2;
      break;
    }
    *b.cr &= ~(FLASH_CR_SER | FLASH_CR_SNB);
  }

  bank_lock(&b);
  return rc;
}

int stm32h7_flash_program(uint32_t addr, const void *src, uint32_t len) {
  if (len == 0U) {
    return 0;
  }
  if ((addr & (STM32H7_FLASH_WORD_BYTES - 1U)) != 0U) {
    return -1; // must start on a flash-word boundary
  }
  flash_bank b;
  bank_for(addr, &b);
  bank_unlock(&b);
  if (bank_wait(&b) != 0) {
    bank_lock(&b);
    return -2;
  }

  const uint8_t *s = (const uint8_t *)src;
  int rc = 0;
  for (uint32_t off = 0; off < len; off += STM32H7_FLASH_WORD_BYTES) {
    // assemble one 256-bit word; pad a short tail with erased-state 0xFF
    uint32_t word[STM32H7_FLASH_WORD_BYTES / 4];
    uint8_t *wb = (uint8_t *)word;
    uint32_t chunk = len - off;
    if (chunk > STM32H7_FLASH_WORD_BYTES) {
      chunk = STM32H7_FLASH_WORD_BYTES;
    }
    memset(wb, 0xFF, STM32H7_FLASH_WORD_BYTES);
    memcpy(wb, s + off, chunk);

    *b.cr |= FLASH_CR_PG;
    volatile uint32_t *dst = (volatile uint32_t *)(addr + off);
    for (unsigned i = 0; i < STM32H7_FLASH_WORD_BYTES / 4; i++) {
      dst[i] = word[i];
    }
    __DSB();
    if (bank_wait(&b) != 0) {
      rc = -3;
      break;
    }
    *b.cr &= ~FLASH_CR_PG;
  }

  bank_lock(&b);
  return rc;
}
