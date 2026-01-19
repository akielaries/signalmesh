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
 */

module pll_example (
  // --- Inputs ---
  input bank1_3v3_xtal_in, // Input clock from the crystal oscillator (27 MHz)
  input bank3_1v8_sys_rst,      // Active-low reset

  // --- Outputs ---
  output pll_clk_out, // Synthesized, faster clock (132 MHz)
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
  .FCLKIN("27"),
  .IDIV_SEL(8), // -> PFD = 3 MHz (range: 3-400 MHz)
  .FBDIV_SEL(43), // -> CLKOUT = 132 MHz (range: 3.125-600 MHz)
  .ODIV_SEL(8) // -> VCO = 1056 MHz (range: 400-1200 MHz)
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
