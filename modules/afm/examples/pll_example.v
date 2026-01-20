//
// pll_example.v
//
// This module demonstrates how to use the generic 'pll_core' block
// to generate two different clock frequencies (84 MHz and 248 MHz)
// from the 27 MHz crystal, exposing both the clocks and their lock signals.
//
`include "include/common_defines.vh"

module pll_example (
  input  bank1_3v3_xtal_in, // 27 MHz crystal input from the board
  input  bank3_1v8_sys_rst, // System reset button

  output bank2_3v3_pll_clk1,   // 84 MHz clock output
  output bank2_3v3_pll_clk2,  // 248 MHz clock output

  output locked_84mhz,     // Lock status for the 84 MHz PLL
  output locked_248mhz      // Lock status for the 248 MHz PLL
);

  wire clk_84mhz_internal;
  wire clk_248mhz_internal;


  // instantiate pll_core for the 248 MHz clock
  pll_core #(
    .FCLKIN(`PLL_248MHZ_FCLKIN),
    .FBDIV_SEL(`PLL_248MHZ_FBDIV_SEL),
    .IDIV_SEL(`PLL_248MHZ_IDIV_SEL),
    .ODIV_SEL(`PLL_248MHZ_ODIV_SEL)
  ) pll_248mhz_inst (
    .clk_in(bank1_3v3_xtal_in),
    .reset_n(bank3_1v8_sys_rst),
    .clk_out(clk_248mhz_internal),
    .locked(locked_248mhz)
  );

  // instantiate pll_core for the 84 MHz clock
  pll_core #(
    .FCLKIN(`PLL_84MHZ_FCLKIN),
    .FBDIV_SEL(`PLL_84MHZ_FBDIV_SEL),
    .IDIV_SEL(`PLL_84MHZ_IDIV_SEL),
    .ODIV_SEL(`PLL_84MHZ_ODIV_SEL)
  ) pll_84mhz_inst (
    .clk_in(bank1_3v3_xtal_in),
    .reset_n(bank3_1v8_sys_rst),
    .clk_out(clk_84mhz_internal),
    .locked(locked_84mhz)
  );


  // --- Output Assignments ---
  assign bank2_3v3_pll_clk1 = clk_84mhz_internal;
  assign bank2_3v3_pll_clk2 = clk_248mhz_internal;

endmodule
