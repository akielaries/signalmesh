/*
 ********************************************************************************************
 * @file      GOWIN_M1_ddr3.c
 * @author    GowinSemicoductor
 * @device    Gowin_EMPU_M1
 * @brief     This file contains all the functions prototypes for the DDR3 firmware library.
 ********************************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "GOWIN_M1_ddr3.h"
#include "debug.h"


/* Definitions ---------------------------------------------------------------*/
uint8_t DDR3_Init(void) {
  uint32_t init_flag = 0;
  uint32_t timeout   = 10000000;

  dbg_printf("DDR3 controller base: 0x%08X\r\n", (uint32_t)DDR3);

  // Force a reset by clearing and setting INIT register
  DDR3->INIT = 0; // Try to reset controller
  for (volatile int i = 0; i < 1000; i++)
    ; // Small delay

  // Check if there's a WR_EN or RD_EN stuck
  dbg_printf("Pre-init: WR_EN=%d RD_EN=%d INIT=%d\r\n", DDR3->WR_EN, DDR3->RD_EN, DDR3->INIT);

  // Clear any pending operations
  DDR3->WR_EN = 0;
  DDR3->RD_EN = 0;

  dbg_printf("Waiting for DDR3 initialization...\r\n");

  do {
    init_flag = DDR3->INIT;
    timeout--;

    if (timeout == 0) {
      dbg_printf("ERROR: DDR3 init TIMEOUT!\r\n");
      return 0xFF;
    }
  } while (!init_flag);

  dbg_printf("SUCCESS: DDR3 initialized! INIT=0x%08X\r\n", init_flag);
  return 1;
}

void DDR3_Read(uint32_t addr, uint32_t *rd_buff) {
  uint32_t *rd_ptr = rd_buff;
  uint32_t rd_en;

  do {
    rd_en = DDR3->RD_EN;
  } while (rd_en); // wait until the rd_en is 0

  DDR3->RD_ADDR = addr;
  DDR3->RD_EN   = 1;

  do {
    rd_en = DDR3->RD_EN;
  } while (rd_en); // wait until the rd_en is 0

  for (int i = 0; i < 4; i++) {
    *rd_ptr = DDR3->RD_DATA[i];
    rd_ptr++;
  }
}

void DDR3_Write(uint32_t addr, uint32_t *wr_buff) {
  uint32_t *wr_ptr = wr_buff;
  uint32_t wr_en;

  do {
    wr_en = DDR3->WR_EN;
  } while (wr_en); // wait until the wr_en is 0

  DDR3->WR_ADDR = addr;

  for (int i = 0; i < 4; i++) {
    DDR3->WR_DATA[i] = *wr_ptr;
    wr_ptr++;
  }

  DDR3->WR_EN = 1;

  do {
    wr_en = DDR3->WR_EN;
  } while (wr_en); // wait until the wr_en is 0,
                   // which means the write operation is finished.
}
