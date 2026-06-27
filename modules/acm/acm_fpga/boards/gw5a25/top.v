// top module for the Audio Creation Module (ACM) on the Gowin GW5A-LV25MG121
//
//   LED[5]   = boot/alive blinker        -> onboard led
//   LED[4:0] = mirror of the scratch register (debug: STM32 writes show up here)
//   UART_TX  = 1 Hz fabric heartbeat (observe on a USB-UART)
//
// the board layer: physical pins, clock, reset, heartbeats, and THIS board's
// identity. the actual datapath (FMC bridge + core/audio regs + DDS) lives in
// the shared acm_top. functionally identical to the GW2AR-18 build; this one
// just runs a 50 MHz clock, uses different pins, and reports fpga_id 0x5025.

module top (
  input  HCLK,            // 50 MHz system clock (ball E2)
  input  HRST,            // reset button on H10 (active high)

  output [5:0] LED,       // LED[5] onboard L6, LED[4:0] on pmod header

  // FMC multiplexed async bus from the STM32 (16-bit AD, addr latched by NADV)
  inout  [15:0] FMC_AD,
  input         FMC_NADV, // NL  (active low) - address latch
  input         FMC_NOE,  //     (active low) - read strobe
  input         FMC_NWE,  //     (active low) - write strobe
  input         FMC_NE1,  //     (active low) - chip select
  output        FMC_NWAIT,//     (active low) - wait (held inactive for PoC)

  output        UART_TX   // 1 Hz heartbeat
);

  localparam integer CLK_FREQ = 50_000_000;   // tang primer 25k system clock
  localparam integer FS_DIV   = 1042;         // 50 MHz / 1042 ~= 48 kHz sample rate

  // boot/alive indicator on LED5
  boot_blinker #(
    .HALF_PERIOD(CLK_FREQ / 2)
  ) boot_blink_inst (
    .HCLK(HCLK),
    .HRST(HRST),
    .BOOT_LED(LED[5])
  );

  // 1 Hz "ACM:XX" fabric heartbeat
  stm32_uart_test #(
    .CLK_FREQ(CLK_FREQ)
  ) uart_test_inst (
    .HCLK(HCLK),
    .HRST(HRST),
    .UART_TX(UART_TX)
  );

  // ---- ACM datapath (FMC regs + synth) - identity declared by this board ----
  wire [15:0] scratch;

  acm_top #(
    .TICK_DIV(FS_DIV)
  ) acm_inst (
    .clk(HCLK),
    .rst(HRST),
    .FMC_AD(FMC_AD),
    .FMC_NADV(FMC_NADV),
    .FMC_NOE(FMC_NOE),
    .FMC_NWE(FMC_NWE),
    .FMC_NE1(FMC_NE1),
    .FMC_NWAIT(FMC_NWAIT),
    .magic_i(16'hACE1),     // common link-check
    .fpga_id_i(16'h5025),   // GW5A-25 (Tang Primer 25K)
    .version_i(16'h0001),
    .scratch_o(scratch)
  );

  // board LEDs are active low; mirror scratch low bits as a write indicator
  assign LED[4:0] = ~scratch[4:0];

endmodule
