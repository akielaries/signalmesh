// top module for the Audio Creation Module (ACM)

module top (
  input  HCLK,        // 27 MHz system clock
  input  HRST,        // S1 button: active-high reset (idles low, high when pressed)

  output [5:0] LED,   // 6 user leds (pads 15-20)
  output UART_TX      // 1 Hz test message to STM32 UART4 RX (PD0), FPGA pin 73
);

  // 1hz boot indicator
  boot_blinker boot_blink_inst (
    .HCLK(HCLK),
    .HRST(HRST),
    .BOOT_LED(LED[5])
  );

  // on board user LEDs blinking
  led_flow #(
    .NUM_LEDS(5)
  ) led_flow_inst (
    .HCLK(HCLK),
    .HRST(HRST),
    .LEDS(LED[4:0])
  );

  // 1hz "ACM:XX" test message out to the STM32 over UART
  stm32_uart_test uart_test_inst (
    .HCLK(HCLK),
    .HRST(HRST),
    .UART_TX(UART_TX)
  );

endmodule
