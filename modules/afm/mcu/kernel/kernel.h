#pragma once

#include <stdint.h>
#include <stddef.h>


#define MAX_THREADS 8

typedef enum {
  PRIO_LOW    = 1,
  PRIO_NORMAL = 2,
  PRIO_HIGH   = 3, // if a thread is HIGH priority it will ALWAYS run first
} thread_prio_e;

typedef enum {
  THREAD_READY = 0,
  THREAD_SLEEPING,
  THREAD_UNUSED
} thread_state_e;

typedef struct thread {
  uint32_t *sp; // saved stack pointer
  volatile uint32_t wake_time;
  thread_state_e state;
  uint8_t priority;
  uint8_t *stack_mem;
  size_t stack_size;
} thread_t;

/* global system time (incremented in systick interrupt handler) */
extern volatile uint32_t system_time_ms;

/*
 * these are needed by the PendSV handler
 */
extern thread_t *volatile current_thread;
thread_t *scheduler_next(void);
extern volatile uint8_t kernel_running;

/* kernel lifecycle */
void kernel_init(void);
void kernel_start(void);

/* thread API */
thread_t *thread_create(thread_t *t,
                        void (*func)(void),
                        uint8_t *stack,
                        size_t stack_size,
                        uint8_t prio,
                        void *arg);

void thread_yield(void);
void thread_sleep_ms(uint32_t ms);


#define THREAD_STACK(name, size)                                               \
  static uint8_t __stack_##name[size];                                         \
  static thread_t __thread_##name

#define THREAD_FUNCTION(name, arg) void name(void)

#define mkthd_static(name, func, size, prio, arg)                              \
  thread_create(&__thread_##name,                                              \
                func,                                                          \
                __stack_##name,                                                \
                sizeof(__stack_##name),                                        \
                prio,                                                          \
                arg)
