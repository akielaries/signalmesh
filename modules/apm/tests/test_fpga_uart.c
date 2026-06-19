// receive the FPGA's "ACM:XX" messages on UART4 using DMA + idle-line detection.
//
// efficient/production pattern: the DMA engine moves every byte with zero CPU
// per byte; we take ONE interrupt per message (when the line goes idle), not
// one per byte. the idle callback (ISR) swaps to the other buffer, restarts the
// DMA, and hands the finished buffer to a worker thread via a mailbox. the
// worker prints it. double-buffered so reception never stalls while printing.

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "bsp/bsp.h"
#include "bsp/utils/bsp_io.h"


#define FPGA_UART (&UARTD4)
#define FPGA_BAUD 1000000     // match stm32_uart_test.v BAUD on the FPGA
#define RXSZ      64U          // max message size; 32-byte aligned multiple



// double buffer for the RX DMA. cache-line aligned: the DMA writes physical AXI
// SRAM and the cortex-m7 d-cache is on, so the worker must invalidate before
// reading (same constraint as the qspi MDMA path)
CC_ALIGN_DATA(32) static uint8_t rxbuf[2][RXSZ];
static volatile uint8_t cur;   // buffer the DMA is currently filling


// mailbox carries (buffer_index << 16) | length from the ISR to the worker
static mailbox_t      rx_mb;
static msg_t          rx_mb_buf[8];


// ISR callback. DMA chunk is done, swap buffers, restart the DMA, hand off the chunk
static void rx_dispatch(UARTDriver *uartp, size_t got) {
  uint8_t done = cur;
  cur ^= 1U;
  uartStartReceiveI(uartp, RXSZ, rxbuf[cur]);   // restart into the other buffer
  if (got > 0U) {
    (void)chMBPostI(&rx_mb, (msg_t)(((uint32_t)done << 16) | (uint32_t)got));
  }
}

// idle line. end of message before the buffer filled
static void rx_idle_cb(UARTDriver *uartp) {
  chSysLockFromISR();
  size_t remaining = uartStopReceiveI(uartp);   // ACTIVE -> stops, returns remaining
  // an idle can fire before main() starts the first receive (the FPGA is
  // already transmitting). only dispatch/restart if a receive was actually
  // active, otherwise leave the driver idle for main()'s initial start
  if (remaining != UART_ERR_NOT_ACTIVE) {
    rx_dispatch(uartp, RXSZ - remaining);
  }
  chSysUnlockFromISR();
}

// buffer completely full (message >= RXSZ; rare here)
static void rx_full_cb(UARTDriver *uartp) {
  chSysLockFromISR();
  rx_dispatch(uartp, RXSZ);                      // rxstate is COMPLETE; got = full buffer
  chSysUnlockFromISR();
}

static void rx_err_cb(UARTDriver *uartp, uartflags_t e) {
  (void)uartp;
  (void)e;                                       // framing/overrun: flags auto-cleared; keep going
}

static const UARTConfig uart_cfg = {
  .txend1_cb  = NULL,
  .txend2_cb  = NULL,
  .rxend_cb   = rx_full_cb,
  .rxchar_cb  = NULL,                            // no per-char callback: DMA moves bytes
  .rxerr_cb   = rx_err_cb,
  .timeout_cb = rx_idle_cb,
  .timeout    = 0,                               // 0 + IDLEIE => idle-line interrupt
  .speed      = FPGA_BAUD,
  .cr1        = USART_CR1_IDLEIE,                // enable the idle-line interrupt
  .cr2        = USART_CR2_STOP1_BITS,
  .cr3        = 0,
};

// worker thread should only wake when a message is queued

static THD_WORKING_AREA(wa_worker, 1024);
static THD_FUNCTION(worker, arg) {
  (void)arg;
  chRegSetThreadName("fpga_rx");

  while (true) {
    msg_t m;
    if (chMBFetchTimeout(&rx_mb, &m, TIME_INFINITE) != MSG_OK) {
      continue;
    }
    uint8_t idx = (uint8_t)(((uint32_t)m >> 16) & 1U);
    size_t  len = (size_t)((uint32_t)m & 0xFFFFU);

    // the DMA wrote this buffer; invalidate before the cpu reads it
    cacheBufferInvalidate(rxbuf[idx], RXSZ);

    // idle framing means one full message per chunk; trim trailing CR/LF
    while (len > 0U &&
           (rxbuf[idx][len - 1] == '\r' || rxbuf[idx][len - 1] == '\n')) {
      len--;
    }
    char tmp[RXSZ + 1];
    memcpy(tmp, rxbuf[idx], len);
    tmp[len] = '\0';
    bsp_printf("FPGA: %s\n", tmp);
  }
}

int main(void) {
  bsp_init();
  bsp_printf("\n--- test_fpga_uart: UART4 DMA + idle-line RX ---\r\n");

  chMBObjectInit(&rx_mb, rx_mb_buf, 8);

  // PD0 = UART4_RX (AF8)
  palSetPadMode(GPIOD, 0, PAL_MODE_ALTERNATE(8));

  uartStart(FPGA_UART, &uart_cfg);
  bsp_printf("UART4 @ %u baud (DMA), waiting for FPGA...\n", (unsigned)FPGA_BAUD);

  chThdCreateStatic(wa_worker, sizeof wa_worker, NORMALPRIO, worker, NULL);

  // kick off the first DMA receive. the ISR restarts it thereafter
  cur = 0U;
  uartStartReceive(FPGA_UART, RXSZ, rxbuf[cur]);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
