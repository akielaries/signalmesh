// boot indicator blinker core (the instantiating top sets board specifics):
//   HRST is ACTIVE HIGH (1 = reset)
//   ACTIVE_LOW=1 means a driven 0 lights the led
// HALF_PERIOD comes from the board top.v as a CLK_FREQ-derived value;
// the default below is only for standalone use and is always overridden

// blinks a single led at ~1 Hz to show the fpga booted and is alive
module boot_blinker #(
  parameter HALF_PERIOD = 27_000_000 / 2,   // default for standalone use only
  parameter ACTIVE_LOW  = 1
) (
  input  wire HCLK,
  input  wire HRST,                         // active high reset
  output wire BOOT_LED
);

  // size the counter from the parameter so any clock frequency works
  reg [$clog2(HALF_PERIOD)-1:0] count;
  reg                           state;

  always @(posedge HCLK or posedge HRST) begin
    if (HRST) begin
      count <= 0;
      state <= 1'b0;
    end
    else if (count == HALF_PERIOD - 1) begin
      count <= 0;
      state <= ~state;
    end
    else begin
      count <= count + 1'b1;
    end
  end

  assign BOOT_LED = ACTIVE_LOW ? ~state : state;

endmodule
