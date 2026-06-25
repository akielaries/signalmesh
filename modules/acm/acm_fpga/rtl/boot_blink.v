// blinker cores for the boot indicator and the user led chase
// conventions (the instantiating top sets the board specifics):
//   HRST is ACTIVE HIGH (1 = reset)
//   ACTIVE_LOW=1 means a driven 0 lights the led
// HALF_PERIOD/STEP_TICKS come from the board top.v as CLK_FREQ-derived values;
// the defaults below are only for standalone use and are always overridden

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

// lights the user leds one after another in a wrapping chase
module led_flow #(
  parameter NUM_LEDS   = 5,
  parameter STEP_TICKS = 27_000_000 / 20,    // 20 times per second
  parameter ACTIVE_LOW = 1
) (
  input  wire                HCLK,
  input  wire                HRST,          // active high reset (S1 pressed)
  output wire [NUM_LEDS-1:0] LEDS
);

  reg [31:0]         tick;
  reg [NUM_LEDS-1:0] onehot;

  always @(posedge HCLK or posedge HRST) begin
    if (HRST) begin
      tick   <= 32'd0;
      onehot <= {{(NUM_LEDS-1){1'b0}}, 1'b1};
    end
    else if (tick == STEP_TICKS - 1) begin
      tick   <= 32'd0;
      // shift the lit position up, wrapping the top bit back to bit 0
      onehot <= {onehot[NUM_LEDS-2:0], onehot[NUM_LEDS-1]};
    end
    else begin
      tick <= tick + 1'b1;
    end
  end

  assign LEDS = ACTIVE_LOW ? ~onehot : onehot;

endmodule
