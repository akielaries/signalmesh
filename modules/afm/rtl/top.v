//
// top.v
//
// This is the top-level design file for the 'afm' module.
// It instantiates all major sub-blocks (PLLs, processors, etc.)
// and connects them to the physical pins of the FPGA.
//
`include "include/common_defines.vh"

module top (
  // --- Physical FPGA Pins ---
  input  bank1_3v3_xtal_in, // 27 MHz crystal input from the board
  input  bank3_1v8_sys_rst, // System reset button

  output bank2_3v3_pll_clk1,   // 84 MHz clock output on a pin
  output bank2_3v3_pll_clk2,  // 248 MHz clock output on a pin

  output [5:0] bank3_1v8_led  // On-board LEDs for status
);

  wire locked_84mhz;
  wire locked_248mhz;


  /**
    * instantiate 2 PLLs for delivering an increased clock
    * TODO: once i come up with a purpose for these clocks beyond just
    * verifying they can work, this instantiation will move to the necessary
    * location
    */
  pll_core #(
    .FCLKIN(`PLL_84MHZ_FCLKIN),
    .FBDIV_SEL(`PLL_84MHZ_FBDIV_SEL),
    .IDIV_SEL(`PLL_84MHZ_IDIV_SEL),
    .ODIV_SEL(`PLL_84MHZ_ODIV_SEL)
  ) pll_84mhz_inst (
    .clk_in(bank1_3v3_xtal_in),
    .reset_n(bank3_1v8_sys_rst),
    .clk_out(pll_clk_out_84mhz),
    .locked(locked_84mhz)
  );

  pll_core #(
    .FCLKIN(`PLL_248MHZ_FCLKIN),
    .FBDIV_SEL(`PLL_248MHZ_FBDIV_SEL),
    .IDIV_SEL(`PLL_248MHZ_IDIV_SEL),
    .ODIV_SEL(`PLL_248MHZ_ODIV_SEL)
  ) pll_248mhz_inst (
    .clk_in(bank1_3v3_xtal_in),
    .reset_n(bank3_1v8_sys_rst),
    .clk_out(pll_clk_out_248mhz),
    .locked(locked_248mhz)
  );


  // --- Status Logic ---
  // Assign PLL lock status to the onboard LEDs
  assign bank3_1v8_led[0] = locked_84mhz;
  assign bank3_1v8_led[1] = locked_248mhz;
  assign bank3_1v8_led[2] = 0;
  assign bank3_1v8_led[3] = 0;
  assign bank3_1v8_led[4] = 0;
  assign bank3_1v8_led[5] = 0;

endmodule
