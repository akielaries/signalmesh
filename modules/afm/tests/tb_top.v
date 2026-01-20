//
// tb_top.v
//
// Testbench for the top-level 'afm' module.
// This file provides simulation stimuli (clock, reset) and
// instantiates the 'top' module for functional verification.
//

// Include common definitions and the actual RTL files.
// Paths are relative to the 'sim' directory.
`include "../include/common_defines.vh"
`include "../cores/pll_core.v" // pll_core is instantiated by top.v
`include "../rtl/top.v"


module tb_top;

  // --- Testbench Parameters ---
  parameter TB_CLOCK_PERIOD = 1_000_000_000 / `CLOCK_FREQ_MHZ; // 1 second / 27 MHz in ps

  // --- Testbench Signals ---
  reg  tb_clk_in;
  reg  tb_sys_rst_n; // Active-low reset

  wire [5:0] tb_led_out;

  // --- Instantiate the Device Under Test (DUT) ---
  top dut (
    .bank1_3v3_xtal_in(tb_clk_in),
    .bank3_1v8_sys_rst(tb_sys_rst_n),
    .bank3_1v8_led(tb_led_out)
  );


  // --- Clock Generation ---
  initial begin
    tb_clk_in = 1'b0;
    forever #(TB_CLOCK_PERIOD / 2) tb_clk_in = ~tb_clk_in; // Generate 27 MHz clock
  end


  // --- Reset Generation and Simulation Control ---
  initial begin
    // Dump waves for waveform viewer (e.g., GTKWave)
    $dumpfile("tb_top.vcd");
    $dumpvars(0, tb_top);

    // Assert reset
    tb_sys_rst_n = 1'b0;
    #(TB_CLOCK_PERIOD * 10); // Hold reset for 10 clock cycles

    // Deassert reset
    tb_sys_rst_n = 1'b1;
    $display("Reset released at %0t", $time);

    // Run simulation for a while
    #(TB_CLOCK_PERIOD * 1_000_000); // Simulate for 1,000,000 reference clock cycles

    $display("Simulation finished at %0t", $time);
    $finish;
  end

  // --- Monitoring/Assertions (Optional) ---
  // You can add always blocks here to monitor signals,
  // check for specific conditions, or make assertions.

endmodule
