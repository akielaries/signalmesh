/*
 * PLL Example for Gowin FPGAs
 *
 * This module demonstrates how to instantiate a Phase-Locked Loop (PLL)
 * to multiply a clock frequency.
 *
 * IMPORTANT: This is a generic example. For a real design, you should use
 * the Gowin IP Core Generator tool in the Gowin EDA to generate a PLL
 * with parameters validated for your specific device and frequency plan.
 * The tool will provide a customized instantiation template.
 *
 * the main idea of a PLL is to lock 
 */

`define PLL_84MHZ_FCLKIN     "27"
`define PLL_84MHZ_IDIV_SEL   8
`define PLL_84MHZ_FBDIV_SEL  27
`define PLL_84MHZ_ODIV_SEL   8

`define PLL_248MHZ_FCLKIN     "27"
`define PLL_248MHZ_IDIV_SEL   4
`define PLL_248MHZ_FBDIV_SEL  45
`define PLL_248MHZ_ODIV_SEL   2



module pll_example (
  input bank1_3v3_xtal_in, // Input clock from the crystal oscillator (27 MHz)
  input bank3_1v8_sys_rst,      // Active-low reset

  output pll_clk_out, // Synthesized, faster clock
  output pll_locked      // High when the PLL output clock is stable
);

wire clk;
wire rst_n;
wire clk_132mhz_internal; // Internal wire for PLL output and feedback

assign clk = bank1_3v3_xtal_in;

assign rst_n = bank3_1v8_sys_rst;
assign pll_clk_out = clk_132mhz_internal; // Assign internal wire to output port



// using the Gowin PLL calculator at:
// juj.github.io/gowin_fpga_code_generators/pll_calculator.html
rPLL #(
  .FCLKIN(`PLL_84MHZ_FCLKIN),
  // -> PFD = (range: 3-400 MHz)
  .IDIV_SEL(`PLL_84MHZ_IDIV_SEL),
  // -> CLKOUT = (range: 3.125-600 MHz)
  .FBDIV_SEL(`PLL_84MHZ_FBDIV_SEL),
  // -> VCO = (range: 400-1200 MHz)
  .ODIV_SEL(`PLL_84MHZ_ODIV_SEL)
)

pll (
  .CLKOUTP(),
  .CLKOUTD(),
  .CLKOUTD3(),
  .RESET(1'b0),
  .RESET_P(1'b0),
  .CLKFB(1'b0),
  .FBDSEL(6'b0),
  .IDSEL(6'b0),
  .ODSEL(6'b0),
  .PSDA(4'b0),
  .DUTYDA(4'b0),
  .FDLY(4'b0),
  .CLKIN(clk), // 27 MHz
  .CLKOUT(pll_clk_out), // 132 MHz
  .LOCK(pll_locked)
);

endmodule
