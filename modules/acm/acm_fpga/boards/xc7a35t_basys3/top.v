// top module for the Audio Creation Module (ACM) on the Digilent Basys 3 (XC7A35T-1CPG236C)
//
//   led[15]   = boot/alive blinker
//   led[14:0] = mirror of the scratch register (STM32 writes show up here)
//   uart_tx   = 1 Hz "ACM:XX" heartbeat on the onboard USB-UART
//
// board layer only: pins, clock, reset, heartbeats and this board's identity. the
// datapath is the shared acm_top, same as the gowin builds. the STM32 link is the
// same 16-bit muxed async FMC bus, wired over pmods JA/JB/JC (see basys3.xdc)

module top (
  input         clk,        // 100 MHz (W5)
  input         btnC,       // centre button as reset (active high)

  output [15:0] led,
  output [3:0]  an,         // 7-seg anodes, held off

  // FMC multiplexed async bus from the STM32 (16-bit AD, addr latched by NADV)
  inout  [15:0] FMC_AD,
  input         FMC_NADV,
  input         FMC_NOE,
  input         FMC_NWE,
  input         FMC_NE1,
  output        FMC_NWAIT,

  output        uart_tx
);

  // the fabric runs at 50 MHz, same as the tang primer 25k: dds_synth is not
  // pipelined for 100 MHz yet (one-cycle lut -> multiply -> mix path)
  localparam integer CLK_FREQ = 50_000_000;
  localparam integer FS_DIV   = 1042;         // 50 MHz / 1042 ~= 48 kHz sample rate

  assign an = 4'b1111;

  // ---- 100 MHz -> 50 MHz (VCO 1000 MHz) ----
  wire clk_fb, clk_fb_buf, clk_50_unbuf, clk_50, mmcm_locked;

  MMCME2_BASE #(
    .CLKIN1_PERIOD(10.0),
    .CLKFBOUT_MULT_F(10.0),
    .CLKOUT0_DIVIDE_F(20.0)
  ) mmcm_inst (
    .CLKIN1(clk),
    .CLKFBIN(clk_fb_buf),
    .CLKFBOUT(clk_fb),
    .CLKOUT0(clk_50_unbuf),
    .LOCKED(mmcm_locked),
    .RST(1'b0),
    .PWRDWN(1'b0),
    .CLKFBOUTB(),
    .CLKOUT0B(),
    .CLKOUT1(),
    .CLKOUT1B(),
    .CLKOUT2(),
    .CLKOUT2B(),
    .CLKOUT3(),
    .CLKOUT3B(),
    .CLKOUT4(),
    .CLKOUT5(),
    .CLKOUT6()
  );

  BUFG fb_bufg (.I(clk_fb), .O(clk_fb_buf));
  BUFG clk_bufg (.I(clk_50_unbuf), .O(clk_50));

  wire rst = btnC | ~mmcm_locked;

  boot_blinker #(
    .HALF_PERIOD(CLK_FREQ / 2),
    .ACTIVE_LOW(0)
  ) boot_blink_inst (
    .HCLK(clk_50),
    .HRST(rst),
    .BOOT_LED(led[15])
  );

  stm32_uart_test #(
    .CLK_FREQ(CLK_FREQ)
  ) uart_test_inst (
    .HCLK(clk_50),
    .HRST(rst),
    .UART_TX(uart_tx)
  );

  wire [15:0] scratch;

  acm_top #(
    .TICK_DIV(FS_DIV)
  ) acm_inst (
    .clk(clk_50),
    .rst(rst),
    .FMC_AD(FMC_AD),
    .FMC_NADV(FMC_NADV),
    .FMC_NOE(FMC_NOE),
    .FMC_NWE(FMC_NWE),
    .FMC_NE1(FMC_NE1),
    .FMC_NWAIT(FMC_NWAIT),
    .magic_i(16'hACE1),
    .fpga_id_i(16'h7035),   // XC7A35T (Basys 3)
    .version_i(16'h0001),
    .scratch_o(scratch)
  );

  // basys 3 leds are active high
  assign led[14:0] = scratch[14:0];

endmodule
