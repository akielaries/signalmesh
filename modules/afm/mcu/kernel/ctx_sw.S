.syntax unified
.thumb
.extern current_thread
.extern scheduler_next

.global PendSV_Handler
.type PendSV_Handler, %function

PendSV_Handler:
    cpsid i

    mrs r0, psp

    ldr r1, =current_thread
    ldr r1, [r1]

    sub r0, #4
    str r7, [r0]
    sub r0, #4
    str r6, [r0]
    sub r0, #4
    str r5, [r0]
    sub r0, #4
    str r4, [r0]

    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11

    sub r0, #4
    str r7, [r0]
    sub r0, #4
    str r6, [r0]
    sub r0, #4
    str r5, [r0]
    sub r0, #4
    str r4, [r0]

    str r0, [r1]

    bl scheduler_next

    ldr r1, =current_thread
    str r0, [r1]

    ldr r0, [r0]

    ldr r4, [r0]
    add r0, #4
    ldr r5, [r0]
    add r0, #4
    ldr r6, [r0]
    add r0, #4
    ldr r7, [r0]
    add r0, #4

    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7

    ldr r4, [r0]
    add r0, #4
    ldr r5, [r0]
    add r0, #4
    ldr r6, [r0]
add r0, #4
    ldr r7, [r0]
    add r0, #4

    msr psp, r0

    cpsie i
    bx lr

.size PendSV_Handler, .-PendSV_Handler
