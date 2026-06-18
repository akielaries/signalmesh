// top modules for the Audio Creation Module (ACM)

module top (

  input HCLK, // hw clock 27MHz
  input HRST, // hw reset
 
  // 4 of the 5 on board LEDs. 5th is the boot LED
  inout [4:0] LED,
  inout BOOT_LED
);

  // boot LED blinker
  boot_blinker boot_blink_module (
    .HCLK(HCLK),
    .HRST(HRST),
    .BOOT_LED_OUT(BOOT_LED)
  );


endmodule