// blinker tasks for the boot indicator and the user led chase
// HRST is the S1 button: on the tang nano 20k pin 88 idles low (external 1k
// pull-down R6) and reads high when pressed, so reset is ACTIVE HIGH
// tang nano 20k leds are active low, so a driven 0 lights the led

// blinks a single led at ~1 Hz to show the fpga booted and is alive
module boot_blinker #(
  parameter HALF_PERIOD = 27_000_000 / 2,   // toggle every 0.5s at 27MHz
  parameter ACTIVE_LOW  = 1
) (
  input  wire HCLK,
  input  wire HRST,                         // active high reset (S1 pressed)
  output wire BOOT_LED
);

  reg [23:0] count;
  reg        state;

  always @(posedge HCLK or posedge HRST) begin
    if (HRST) begin
      count <= 24'd0;
      state <= 1'b0;
    end
    else if (count == HALF_PERIOD - 1) begin
      count <= 24'd0;
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
  parameter STEP_TICKS = 27_000_000 / 20,    // advance ~10 times per second
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
