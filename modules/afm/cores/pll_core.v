//
// pll_core.v
//
// Generic, configurable PLL core for generating various clock frequencies.
// This module wraps the vendor-specific 'rPLL' hardware primitive/IP
//
`include "include/common_defines.vh"


module pll_core #(
  // --- Parameters to make this block dynamic ---
  // These values can be overridden when the module is instantiated.
  parameter FCLKIN    = `CLOCK_FREQ_MHZ,
  parameter FBDIV_SEL = 19, // Feedback Divider (Multiplier)
  parameter IDIV_SEL  = 0,  // Input Divider
  parameter ODIV_SEL  = 5   // Output Divider
)(
  // --- Interface ---
  input  clk_in,      // Input clock (typically from the main crystal)
  input  reset_n,     // Active-low system reset

  output clk_out,     // The newly generated clock
  output locked       // High when the PLL output is stable
);

  // --- IP Core Instantiation ---
  // Instantiate the Gowin 'rPLL' hard IP core
  rPLL #(
    .FCLKIN(FCLKIN),
    .IDIV_SEL(IDIV_SEL),
    .FBDIV_SEL(FBDIV_SEL),
    .ODIV_SEL(ODIV_SEL)
  ) rpll_inst (
    // Core Clocking and Reset
    .CLKIN(clk_in),
    .RESET(!reset_n), // The primitive requires an active-high reset
    .RESET_P(1'b0),     // Power-on reset for dynamic re-configuration
    // Main Outputs
    .CLKOUT(clk_out),
    .LOCK(locked),
    // Unused Optional Outputs
    .CLKOUTP(),
    .CLKOUTD(),
    .CLKOUTD3(),
    // Feedback Control (unused in this static configuration)
    .CLKFB(1'b0),       // Set to 0 to use internal feedback path
    .FBDSEL(6'b0),
    // Dynamic Re-configuration Ports (unused)
    .IDSEL(6'b0),
    .ODSEL(6'b0),
    .PSDA(4'b0),
    .DUTYDA(4'b0),
    .FDLY(4'b0)
  );

endmodule
