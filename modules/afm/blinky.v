/**
 * simple blinky program
 */


// (6749999+1)÷27000000 = 0.25s
`define TIM_DIV_250MS 32'd6_749_999
// (13499999+1)÷27000000 = 0.5s
`define TIM_DIV_500MS 32'd13_499_999



module blinky (
  input bank1_3v3_xtal_in,        // 27 MHz clock input
  input bank3_1v8_sys_rst,        // active-low reset input
  output reg bank2_3v3_red_led,
  output reg [5:0] bank3_1v8_led, // 6-bit output driving LED pins
  output bank1_3v3_xtal_route,    // route crystal to a GPIO
);


wire clk;
wire rst_n; // active low reset line

assign clk = bank1_3v3_xtal_in;
assign rst_n = bank3_1v8_sys_rst;
assign bank1_3v3_xtal_route = clk;



reg [31:0] led_counter;       // LED shifting counter
reg [31:0] red_led_counter;   // Red LED blinking counter

// LED and bank2_3v3_red_led counter logic
always @(posedge clk or negedge rst_n) begin
  if (!rst_n) begin
    led_counter <= 32'd0;
    red_led_counter <= 32'd0;
  end else begin
    // - increment both independently. the LEDs on the board at a cadence
    // of 2hz and the off-board LED at 1hz
    // - each cycle is toggling the LED. so first pass is on, second is off
    // meaning 2 cycles for a "blink". on for N seconds, off for N seconds

    if (led_counter < `TIM_DIV_250MS)
      led_counter <= led_counter + 1'd1;
    else
      led_counter <= 32'd0;

    if (red_led_counter < `TIM_DIV_500MS)
      red_led_counter <= red_led_counter + 1'd1;
    else
      red_led_counter <= 32'd0;
  end
end

// LED shifting and bank2_3v3_red_led blinking logic
always @(posedge clk or negedge rst_n) begin
  if (!rst_n) begin
    bank3_1v8_led <= 6'b111110;
    bank2_3v3_red_led <= 1'b0;
  end else begin
    // Shift the LED bits when led_counter hits max
    if (led_counter == `TIM_DIV_250MS)
      bank3_1v8_led <= {bank3_1v8_led[4:0], bank3_1v8_led[5]};

    // Toggle bank2_3v3_red_led when red_led_counter hits max
    if (red_led_counter == `TIM_DIV_500MS)
      bank2_3v3_red_led <= ~bank2_3v3_red_led;
  end
end

endmodule
