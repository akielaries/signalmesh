/*
 * Flexible Clock Generator (NCO) Example for Gowin FPGAs
 *
 * This module uses a Numerically Controlled Oscillator (NCO) to generate
 * a square wave of a given frequency. The frequency is controlled by the
 * PHASE_INCREMENT parameter.
 *
 */

module clk_div_example #(
  // This parameter controls the output frequency.
  // F_out = (F_clk * PHASE_INCREMENT) / (2^32)
  // For F_out=30kHz, PHASE_INCREMENT = (30000 * 2^32) / 27000000 = 4772186
  parameter PHASE_INCREMENT = 32'd4_772_186 // 30khz
  //parameter PHASE_INCREMENT = 32'd19_088_744 // 120khz
  //parameter PHASE_INCREMENT = 32'd1_908_874_353 // 12mhz
  //parameter PHASE_INCREMENT = 32'd159_072_862 // 1mhz
  //parameter PHASE_INCREMENT = 32'd318_145_725 // 2mhz
  //parameter PHASE_INCREMENT = 32'd477_218_588 // 3mhz
  //parameter PHASE_INCREMENT = 32'd636_291_451 // 4mhz

  //parameter PHASE_INCREMENT = 32'd795_364_314 // 5mhz

)(
  // --- Inputs ---
  input bank1_3v3_xtal_in, // Input clock from the crystal oscillator (27 MHz)
  input bank3_1v8_sys_rst, // Active-low reset

  // --- Outputs ---
  //output bank1_3v3_xtal_route,
  output reg clk_div_out      // The new, generated clock output
);

  wire clk;
  wire rst_n;

  assign clk = bank1_3v3_xtal_in;
  //assign bank1_3v3_xtal_route = bank1_3v3_xtal_in;
  assign rst_n = bank3_1v8_sys_rst;
  // --------------------------------------------------------

  // numerically controller oscillator (NCO)
  reg [31:0] phase_accumulator;

  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      phase_accumulator <= 32'd0;
    end else begin
      phase_accumulator <= phase_accumulator + PHASE_INCREMENT;
    end
  end

  // The output clock is the most significant bit (MSB) of the accumulator,
  // passed through an output register for signal integrity.
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      clk_div_out <= 1'b0;
    end else begin
      clk_div_out <= phase_accumulator[31];
    end
  end

endmodule
