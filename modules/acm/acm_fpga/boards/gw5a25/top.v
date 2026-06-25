// top module for the Audio Creation Module (ACM) on the Gowin GW5A-LV25MG121
//
//   LED[5]   = boot/alive blinker        -> onboard led
//   LED[4:0] = LED_CTRL register written by the STM32 over FMC -> pmod header
//   UART_TX  = 1 Hz fabric heartbeat (observe on a USB-UART)
//
// shares the cores in ../../rtl; only this integration layer and the pin/timing
// constraints are board specific. functionally identical to the GW2AR-18 build,
// the GW5A-25 just runs a 50 MHz clock and uses different pins

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
