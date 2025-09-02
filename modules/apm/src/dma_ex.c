#include "ch.h"
#include "hal.h"
#include "chprintf.h"


#define DMA_STREAM_ID STM32_DMA_STREAM_ID(1, 2)
#define DMA_CHANNEL   3 // Channel tied to TIM2_UP (check your MCU RM)
#define DMA_PRIORITY  2

static BaseSequentialStream *chp = (BaseSequentialStream *)&SD5;

static const SerialConfig uart5_cfg = {
  .speed = 1000000,
  .cr1   = 0,
  .cr2   = 0,
  .cr3   = 0,
};

static uint16_t samples[64]; // DMA destination buffer
static volatile bool dmaComplete = false;

/* DMA ISR handler */
static void dmaCompleteISR(void *p, uint32_t flags) {
  (void)p;

  if (flags & (STM32_DMA_ISR_TCIF)) {
    dmaComplete = true;
  } else if (flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) {
    chSysHalt("DMA error");
  }
}

static void dmaStart(void) {
  bool b;

  osalSysLock();
  b = dmaStreamAlloc(DMA_STREAM, DMA_PRIORITY, dmaCompleteISR, NULL);
  osalDbgAssert(!b, "DMA already allocated");

  dmaStreamSetPeripheral(DMA_STREAM, &TIM2->CNT); // Source: timer counter
  dmaStreamSetMemory0(DMA_STREAM, samples);       // Destination: RAM buffer
  dmaStreamSetTransactionSize(DMA_STREAM, 64);

  uint32_t mode = STM32_DMA_CR_CHSEL(DMA_CHANNEL) |
                  STM32_DMA_CR_PL(DMA_PRIORITY) | STM32_DMA_CR_DIR_P2M |
                  STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_PSIZE_HWORD |
                  STM32_DMA_CR_MINC | STM32_DMA_CR_TCIE | STM32_DMA_CR_DMEIE |
                  STM32_DMA_CR_TEIE;

  dmaStreamSetMode(DMA_STREAM, mode);
  dmaStreamEnable(DMA_STREAM);
  osalSysUnlock();
}

static THD_WORKING_AREA(waBlinker, 512);
static THD_FUNCTION(ThreadBlinker, arg) {
  (void)arg;
  chRegSetThreadName("blinker");

  while (!dmaComplete) {
    chThdSleepMilliseconds(100);
    chprintf(chp, "Waiting for DMA...\r\n");
  }

  chprintf(chp, "DMA complete. Values:\r\n");
  for (int i = 0; i < 64; ++i) {
    chprintf(chp, "samples[%d] = %u\r\n", i, samples[i]);
  }

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}

int main(void) {
  halInit();
  chSysInit();

  sdStart(&SD5, &uart5_cfg);

  // Start TIM2 with no prescaler, running freely
  rccEnableTIM2(TRUE);
  TIM2->PSC = 0;
  TIM2->ARR = 0xFFFF;
  TIM2->CR1 = TIM_CR1_CEN;

  dmaStart();

  chThdCreateStatic(waBlinker,
                    sizeof(waBlinker),
                    NORMALPRIO,
                    ThreadBlinker,
                    NULL);

  while (true) {
    chThdSleep(TIME_INFINITE);
  }
}
