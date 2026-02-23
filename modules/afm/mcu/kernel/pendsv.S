.syntax unified
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

// implemented in kernel.c. can I include headers in this? idk
.extern current_thread
.extern scheduler_next


/*
 * Every context switch goes through here. There are two cases:
 *
 **Case 1: First ever switch (current_thread == NULL)**
 **Case 2: Normal switch (current_thread != NULL)**
 *
 *
 *
 * the manual states:
 * > Registers R8-R12 are not accessible by all 16-bit instructions
 *
 * PSP stack layout after exception entry:
 *
 *  (high addr)
 *  +-----------+
 *  |   xPSR    |  status flags (Thumb bit 0x01000000)
 *  |    PC     |  program counter: address of next instruction in interrupted thread
 *  |    LR     |  link reg: return address of interrupted function
 *  |   R12     |  scratch register (caller-saved?)
 *  |    R3     |  \
 *  |    R2     |  |  argument / scratch registers
 *  |    R1     |  |  (caller-saved, may hold anything?)
 *  |    R0     |  /
 *  +-----------+  <-- PSP on exception entry (CPU did this automatically)
 *  |    R7     |  \
 *  |    R6     |  |
 *  |    R5     |  |  callee-saved registers.....thread is responsible for
 *  |    R4     |  |  preserving these across function calls, so we must
 *  |   R11     |  |  save/restore them manually across context switches
 *  |   R10     |  |
 *  |    R9     |  |
 *  |    R8     |
 *  +-----------+  <-- thread->sp (saved here, restored on next switch in)
 *  (low addr)
 */


PendSV_Handler:
  // critical section so we disable interrupts here
  // i think this means systick doesn't fire? is that bad?
  cpsid   i
  // save R4 to later overwrite/clobber and the link register is going to contain the
  // EXC_RETURN value we need to return from exception
  // there is something about 8byte alignment here
  push    {r4, lr}

  // ---------------------------------------------------------------
  // SAVE CURRENT THREAD CONTEXT
  // skip if current_thread == NULL (first ever switch, nothing to save)
  // ---------------------------------------------------------------

  // r2 = &current_thread (address of the pointer)
  ldr     r2, =current_thread
  // r3 = current_thread  (the thread_t* itself)
  ldr     r3, [r2]
  // make sure it's not NULL?
  // r3 - 0 
  cmp     r3, #0
  // first context switch, no thread to save
  beq     skip_save

  mrs     r0, psp
  subs    r0, r0, #32

  movs    r1, r0
  adds    r1, #16
  stm     r1!, {r4-r7}        // save real r4-r7 at +16

  mov     r4, r8
  mov     r5, r9
  mov     r6, r10
  mov     r7, r11
  stm     r0!, {r4-r7}        // save r8-r11 at +0

  subs    r0, r0, #16         // r0 back to frame base
  str     r0, [r3]            // t->sp = r0

skip_save:
wait_for_ready:
  cpsie   i
  wfi
  cpsid   i
  bl      scheduler_next
  cmp     r0, #0
  beq     wait_for_ready

  ldr     r2, =current_thread
  str     r0, [r2]            // current_thread = next
  ldr     r0, [r0]            // r0 = next->sp

  ldm     r0!, {r4-r7}        // restore r8-r11 (stored at low addr)
  mov     r8, r4
  mov     r9, r5
  mov     r10, r6
  mov     r11, r7

  ldm     r0!, {r4-r7}        // restore r4-r7

  msr     psp, r0
  cpsie   i
  pop     {r4, pc}            // restore saved r4, and EXC_RETURN -> pc
