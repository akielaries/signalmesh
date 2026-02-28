#include "GOWIN_M1.h"

#include "kernel.h"
#include "debug.h"
#include "gpio.h"
#include "delay.h"
#include "sys_defs.h"
#include "GOWIN_M1_ddr3.h"

#include "hw.h"

#include <stdint.h>


// dummy generator
static uint32_t lcg_state = 12345;

static uint32_t lcg_rand(void) {
  lcg_state = lcg_state * 1664525 + 1013904223;
  return lcg_state;
}

/* ========================================================= */

// IDLE THREAD! this should ideally just be main() i think but idk
THREAD_STACK(idle, 256);
THREAD_FUNCTION(idle_fn, arg) {
  while (1) {
    __WFI();
  }
}

THREAD_STACK(uptime, 512);
THREAD_FUNCTION(uptime_fn, arg) {
  while (1) {
    dbg_printf("uptime: %ds\r\n", system_time_ms / 1000);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(blink1_thd, 512);
THREAD_FUNCTION(blink1_fn, arg) {
  while (1) {
    dbg_printf("pin0\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_0);
    thread_sleep_ms(500);
  }
}

THREAD_STACK(blink2_thd, 512);
THREAD_FUNCTION(blink2_fn, arg) {
  while (1) {
    dbg_printf("pin1\r\n");
    GPIO_ToggleBit(GPIO0, GPIO_Pin_1);
    thread_sleep_ms(1000);
  }
}

THREAD_STACK(fast_thd, 256);
THREAD_FUNCTION(fast_fn, arg) {
  while (1) {
    GPIO_ToggleBit(GPIO0, GPIO_Pin_2);
    //thread_sleep_ms(2);
  }
}

// basic thread that just yields
// some CPU heavy task that will spit out the result every few seconds or
// something related
THREAD_STACK(compute_thd, 512);
THREAD_FUNCTION(compute_fn, arg) {
  while (1) {
    uint32_t iters = 500000 + (lcg_rand() % 5000000);
    uint32_t result = 0;

    uint32_t t_start = system_time_ms;

    for (uint32_t i = 0; i < iters; i++) {
      result += i * i;
      if ((i % 1000) == 0)
        thread_yield(); // could remove this manual yield
    }

    uint32_t elapsed = system_time_ms - t_start;
    dbg_printf("compute result: %u, took %d ms (%d iters)\r\n", result, elapsed, iters);
  }
}

void ddr3_pattern_test(void) {
    dbg_printf("\r\n=== DDR3 Pattern Test ===\r\n");

    uint32_t errors = 0;

    // Make buffers static to avoid stack issues
    static uint32_t write_buf[4];
    static uint32_t read_buf[4];

    // Write pattern to 64 different 16-byte blocks (1KB total)
    dbg_printf("Writing pattern to 64 blocks (1KB)...\r\n");
    for (uint32_t block = 0; block < 64; block++) {
        // Fill buffer with pattern
        write_buf[0] = (block * 4) + 0;
        write_buf[1] = (block * 4) + 1;
        write_buf[2] = (block * 4) + 2;
        write_buf[3] = (block * 4) + 3;

        uint32_t addr = block * 16;  // 16-byte aligned addresses
        DDR3_Write(DDR3_BASE + addr, write_buf);

        if ((block % 16) == 0) {
            dbg_printf("  Written %d blocks...\r\n", block);
        }
    }

    dbg_printf("Reading back and verifying...\r\n");
    for (uint32_t block = 0; block < 64; block++) {
        uint32_t addr = block * 16;
        DDR3_Read(DDR3_BASE + addr, read_buf);

        // Verify each word in the block
        for (int word = 0; word < 4; word++) {
            uint32_t expected = (block * 4) + word;
            if (read_buf[word] != expected) {
                dbg_printf("  ERROR at addr 0x%08X[%d]: expected 0x%08X, read 0x%08X\r\n",
                           DDR3_BASE + addr, word, expected, read_buf[word]);
                errors++;
                if (errors > 10) goto done;
            }
        }

        if ((block % 16) == 0) {
            dbg_printf("  Verified %d blocks...\r\n", block);
        }
    }

done:
    if (errors == 0) {
        dbg_printf("PASS: Pattern test succeeded! All 256 words correct.\r\n");
    } else {
        dbg_printf("FAIL: Pattern test failed with %d errors\r\n", errors);
    }

    // Show first 4 blocks (64 bytes = 16 words)
    dbg_printf("\r\nFirst 4 blocks at DDR3_BASE:\r\n");
    for (uint32_t block = 0; block < 4; block++) {
        DDR3_Read(DDR3_BASE + (block * 16), read_buf);
        dbg_printf("  Block %d [0x%08X]: 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
                   block, DDR3_BASE + (block * 16),
                   read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
    }
}

void ddr3_rw_test(void) {
  dbg_printf("DDR3 test (16-byte aligned)\r\n");
  
  static uint32_t write_data[6][4] = {
    {0x91234567, 0x89abcdef, 0xfedcba98, 0x76543210},
    {0x66666666, 0x88888888, 0xeeeeeeee, 0xffffffff},
    {1, 2, 3, 4}, 
    {5, 6, 7, 8}, 
    {9, 10, 11, 12},
    {13, 14, 15, 16} 
  };
  
  static uint32_t read_data[6][4];
  
  unsigned int addr = 0x00;
  
  dbg_printf("Writing 6 blocks with 16-byte increment...\r\n");
  for(int i = 0; i < 6; i++) {
    DDR3_Write(addr + i*16, write_data[i]);  // Changed to i*16
  }
  
  dbg_printf("Reading back...\r\n");
  for(int i = 0; i < 6; i++) {
    DDR3_Read(addr + i*16, read_data[i]);    // Changed to i*16
  }
  
  dbg_printf("Results:\r\n");
  for(int i = 0; i < 6; i++) {
    dbg_printf("Block %d: 0x%08X 0x%08X 0x%08X 0x%08X ", 
               i, read_data[i][0], read_data[i][1], read_data[i][2], read_data[i][3]);
    
    // Check if correct
    if (read_data[i][0] == write_data[i][0] &&
        read_data[i][1] == write_data[i][1] &&
        read_data[i][2] == write_data[i][2] &&
        read_data[i][3] == write_data[i][3]) {
      dbg_printf("OK\r\n");
    } else {
      dbg_printf("MISMATCH\r\n");
    }
  }
}

/* ========================== MAIN ========================= */
/* ========================================================= */
/*
 * this is the main entry point for the application. it initializes the system and the threads
 */
int main(void) {
  hw_init();

  dbg_printf("DDR3 init\r\n");
  uint8_t status = DDR3_Init();
  dbg_printf("DDR3 init status: %d\r\n", status);

  ddr3_rw_test();
  //ddr3_pattern_test();

  // start the scheduler/kernel
  dbg_printf("initializing kernel...\r\n");
  kernel_init();

  dbg_printf("creating threads...\r\n");

  // idle thread with lowest priority
  mkthd_static(idle, idle_fn, sizeof(idle), PRIO_LOW, NULL);
  mkthd_static(uptime, uptime_fn, sizeof(uptime), PRIO_NORMAL, NULL);

  mkthd_static(blink1_thd, blink1_fn, sizeof(blink1_thd), PRIO_NORMAL, NULL);
  mkthd_static(blink2_thd, blink2_fn, sizeof(blink2_thd), PRIO_NORMAL, NULL);

  mkthd_static(fast_thd, fast_fn, sizeof(fast_thd), PRIO_LOW, NULL);
  mkthd_static(compute_thd, compute_fn, sizeof(compute_thd), PRIO_LOW, NULL);


  dbg_printf("system_time_ms before start: %d\r\n", system_time_ms);

  dbg_printf("starting kernel...\r\n");
  kernel_start();

  dbg_printf("main loop...\r\n");
  while (1) {
    thread_sleep_ms(100);
  }
  return 0;
}
