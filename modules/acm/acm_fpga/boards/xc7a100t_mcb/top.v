// top module for the MAIN_COMPUTE_BOARD v1_r1 (XC7A100T-2FGG484)
//
// bring-up stub until the 32-bit synchronous FMC bridge exists. it proves the pin
// plan, clocking and bitstream flow, and leaves the STM32 bus quiet:
//
//   fpga_led[0] = 1 Hz heartbeat from sysclk_100m
//   fpga_led[1] = fpga_btn[0], fpga_led[2] = fpga_btn[1]
//   fpga_led[3] = fpga_rst_n held low by the STM32
//   fmc_d       = high-z, fmc_nwait = inactive, fpga_irq = 0
//
// next: replace the stub with acm_top on a sync 32-bit bridge (fpga_id 0x7100)

module top (
  input         sysclk_100m,

  output [3:0]  fpga_led,
  input  [1:0]  fpga_btn,

  // STM32 FMC bank 1, synchronous NOR/PSRAM, 32-bit, non-multiplexed
  input         fmc_clk,
  inout  [31:0] fmc_d,
  input  [18:0] fmc_a,
  input  [3:0]  fmc_nbl,
  input         fmc_ne1,
  input         fmc_noe,
  input         fmc_nwe,
  output        fmc_nwait,

  output [5:0]  fpga_irq,
  input         fpga_rst_n
);

  localparam integer CLK_FREQ = 100_000_000;

  boot_blinker #(
    .HALF_PERIOD(CLK_FREQ / 2),
    .ACTIVE_LOW(0)
  ) boot_blink_inst (
    .HCLK(sysclk_100m),
    .HRST(~fpga_rst_n),
    .BOOT_LED(fpga_led[0])
  );

  assign fpga_led[1] = fpga_btn[0];
  assign fpga_led[2] = fpga_btn[1];
  assign fpga_led[3] = ~fpga_rst_n;

  assign fmc_d     = 32'bz;
  assign fmc_nwait = 1'b1;
  assign fpga_irq  = 6'b0;

endmodule
