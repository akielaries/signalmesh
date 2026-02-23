/*
 ******************************************************************************************
 * @file      GOWIN_M1_it.c
 * @author    GowinSemicoductor
 * @device    Gowin_EMPU_M1
 * @brief     Main Interrupt Service Routines.
 *            This file provides template for all exceptions handler and
 *            peripherals interrupt service routine.
 ******************************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "GOWIN_M1_it.h"
#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"


/* Definitions ---------------------------------------------------------------*/
/** @addtogroup GOWIN_M1_StdPeriph_Template
 * @{
 */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

// extern thread_t *current_thread;
// extern thread_t *scheduler_next(void);

/******************************************************************************/
/*            Cortex-M1 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  none
 * @retval none
 */
void NMI_Handler(void) {}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  none
 * @retval none
 */
void HardFault_Handler(void) {
  register uint32_t msp __asm("r0");
  __asm volatile("mrs r0, msp" : "=r"(msp));
  dbg_printf("FAULT msp=0x%08X\r\n", msp);
  __asm volatile("bkpt #0"); // breakpoint for gdb!

  /* Go to infinite loop when Hard Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  none
 * @retval none
 */
void SVC_Handler(void) {
  /*
   * kernel_start initiates the first context switch by calling svc.
   * this handler just needs to pend the pendsv interrupt to get
   * the scheduler running.
   */
  // dbg_printf("SVC_Handler\r\n");
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  none
 * @retval none
 */
void SysTick_Handler(void) {
  system_time_ms++;

  // the kernel might not be running! this is only initially though on boot i
  // think?
  if (kernel_running) {
    // send us to pendSV to service anything that's "pending"? this is essentially
    // the "preemption" part of this system w/ a quantum of 1ms....
    // this fires every 1ms at highest priority, preempting whatever thread is
    // running. It increments time and pends PendSV. Since PendSV is lowest
    // priority it won't actually run until SysTick returns, but that's fine
    // the pending bit stays set. This is what makes the scheduler preemptive
    // rather than cooperative. A thread can't hold the CPU longer than 1ms
    // regardless of whether it yields.
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  }
}

/******************************************************************************/
/*                 GOWIN_M1 Peripherals Interrupt Handlers */
/*  Add here the Interrupt Handler for the used peripheral(s) (XXX), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_GOWIN_M1.s). */
/******************************************************************************/

/**
 * @brief  This function handles UART0 interrupt request.
 * @param  none
 * @retval none
 */
void UART0_Handler(void) {}

/**
 * @brief  This function handles UART1 interrupt request.
 * @param  none
 * @retval none
 */
void UART1_Handler(void) {}

/**
 * @brief  This function handles TIMER0 interrupt request.
 * @param  none
 * @retval none
 */
void TIMER0_Handler(void) {}

/**
 * @brief  This function handles TIMER1 interrupt request.
 * @param  none
 * @retval none
 */
void TIMER1_Handler(void) {}

/**
 * @brief  This function handles GPIO0_0~15 combine interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_Handler(void) {}

/**
 * @brief  This function handles RTC interrupt request.
 * @param  none
 * @retval none
 */
void RTC_Handler(void) {}

/**
 * @brief  This function handles I2C interrupt request.
 * @param  none
 * @retval none
 */
void I2C_Handler(void) {}

/**
 * @brief  This function handles CAN interrupt request.
 * @param  none
 * @retval none
 */
void CAN_Handler(void) {}

/**
 * @brief  This function handles Ethernet interrupt request.
 * @param  none
 * @retval none
 */
void ENT_Handler(void) {}

/**
 * @brief  This function handles DualTimer interrupt request.
 * @param  none
 * @retval none
 */
void DTimer_Handler(void) {}

/**
 * @brief  This function handles TRNG interrupt request.
 * @param  none
 * @retval none
 */
void TRNG_Handler(void) {}

/**
 * @brief  This function handles GPIO0_0 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_0_Handler(void) {}

/**
 * @brief  This function handles GPIO0_1 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_1_Handler(void) {}

/**
 * @brief  This function handles GPIO0_2 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_2_Handler(void) {}

/**
 * @brief  This function handles GPIO0_3 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_3_Handler(void) {}

/**
 * @brief  This function handles GPIO0_4 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_4_Handler(void) {}

/**
 * @brief  This function handles GPIO0_5 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_5_Handler(void) {}

/**
 * @brief  This function handles GPIO0_6 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_6_Handler(void) {}

/**
 * @brief  This function handles GPIO0_7 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_7_Handler(void) {}

/**
 * @brief  This function handles GPIO0_8 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_8_Handler(void) {}

/**
 * @brief  This function handles GPIO0_9 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_9_Handler(void) {}

/**
 * @brief  This function handles GPIO0_10 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_10_Handler(void) {}

/**
 * @brief  This function handles GPIO0_11 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_11_Handler(void) {}

/**
 * @brief  This function handles GPIO0_12 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_12_Handler(void) {}

/**
 * @brief  This function handles GPIO0_13 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_13_Handler(void) {}

/**
 * @brief  This function handles GPIO0_14 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_14_Handler(void) {}

/**
 * @brief  This function handles GPIO0_15 interrupt request.
 * @param  none
 * @retval none
 */
void GPIO0_15_Handler(void) {}

/**
 * @brief  This function handles external 0 interrupt request.
 * @param  none
 * @retval none
 */
void EXTINT_0_Handler(void) {}

/**
 * @brief  This function handles external 1 interrupt request.
 * @param  none
 * @retval none
 */
void EXTINT_1_Handler(void) {}

/**
 * @brief  This function handles external 2 interrupt request.
 * @param  none
 * @retval none
 */
void EXTINT_2_Handler(void) {}

/**
 * @brief  This function handles external 3 interrupt request.
 * @param  none
 * @retval none
 */
void EXTINT_3_Handler(void) {}

/**
 * @}
 */
