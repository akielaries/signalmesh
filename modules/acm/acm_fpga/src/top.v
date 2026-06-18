// top module for the Audio Creation Module (ACM)

module top (
  input  HCLK,        // 27 MHz system clock
  input  HRST,        // S1 button: active-high reset (idles low, high when pressed)

  output [5:0] LED    // 6 user leds (pads 15-20)
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

endmodule
