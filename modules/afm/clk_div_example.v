/*
 * Clock Divider Example for Gowin FPGAs
 *
 * This module demonstrates a simple clock divider. It takes the 27 MHz
 * master clock and divides it by 4 to produce a 6.75 MHz clock.
 *
 */

module clk_div_example (
  // --- Inputs ---
  input bank1_3v3_xtal_in, // Input clock from the crystal oscillator (27 MHz)
  input bank3_1v8_sys_rst, // Active-low reset

  // --- Outputs ---
  output bank1_3v3_xtal_route,
  output clk_div_out      // The new, divided clock output (6.75 MHz)
);

  // --- Port Aliases (standard Verilog via wire/assign) ---
  wire clk;
  wire rst_n;

  assign clk = bank1_3v3_xtal_in;
  assign bank1_3v3_xtal_route = bank1_3v3_xtal_in;
  assign rst_n = bank3_1v8_sys_rst;
  // --------------------------------------------------------

  // --- Clock Divider Logic ---
  // A 2-bit counter is used to achieve a divide-by-4.
  // The counter will cycle through 00, 01, 10, 11 on each clock edge.
  reg [1:0] counter;

  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      counter <= 2'd0;
    end else begin
      counter <= counter + 1'b1;
    end
  end

  // The divided clock is simply the most significant bit (MSB) of the counter.
  // The MSB will be low for two cycles and high for two cycles, creating a
  // new clock with a 50% duty cycle at 1/4 the frequency of the input clock.
  assign clk_div_out = counter[1];

endmodule
