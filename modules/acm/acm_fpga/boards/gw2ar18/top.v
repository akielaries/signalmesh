// top module for the Audio Creation Module (ACM)
//
//   LED[5]   = boot/alive blinker
//   LED[4:0] = LED_CTRL register written by the STM32 over FMC
//   UART_TX  = 1 Hz fabric heartbeat (observe on a USB-UART)

module top (
  input  HCLK,            // 27 MHz system clock
  input  HRST,            // S1 button: active-high reset

  output [5:0] LED,       // 6 user leds (pads 15-20)

  // FMC multiplexed async bus from the STM32 (16-bit AD, addr latched by NADV)
  inout  [15:0] FMC_AD,
  input         FMC_NADV, // NL  (active low) - address latch
  input         FMC_NOE,  //     (active low) - read strobe
  input         FMC_NWE,  //     (active low) - write strobe
  input         FMC_NE1,  //     (active low) - chip select
  output        FMC_NWAIT,//     (active low) - wait (held inactive for PoC)

  output        UART_TX   // 1 Hz heartbeat on FPGA pin 73
);

  localparam integer CLK_FREQ = 27_000_000;   // tang nano 20k system clock

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

  // FMC register slave
  wire [15:0] led_ctrl;
  fmc_mux_slave fmc_inst (
    .clk(HCLK),
    .rst(HRST),
    .AD(FMC_AD),
    .NADV(FMC_NADV),
    .NOE(FMC_NOE),
    .NWE(FMC_NWE),
    .NE1(FMC_NE1),
    .NWAIT(FMC_NWAIT),
    .led_ctrl(led_ctrl)
  );

  // board LEDs are active low; invert so a written 1 lights the led
  assign LED[4:0] = ~led_ctrl[4:0];

endmodule
