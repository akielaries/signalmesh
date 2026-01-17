module blinky (
  input bank1_3v3_xtal_in,          // 27 MHz clock input
  input bank3_1v8_sys_rst,        // active-low reset input
  output reg bank2_3v3_red_led,
  output reg [5:0] bank3_1v8_led    // 6-bit output driving LED pins
);

reg [23:0] led_counter;       // LED shifting counter
reg [24:0] red_led_counter;   // Red LED blinking counter

// LED and bank2_3v3_red_led counter logic
always @(posedge bank1_3v3_xtal_in or negedge bank3_1v8_sys_rst) begin
  if (!bank3_1v8_sys_rst) begin
    led_counter <= 24'd0;
    red_led_counter <= 25'd0;
  end else begin
    // increment both independently
    if (led_counter < 24'd674_9999)
      led_counter <= led_counter + 1'd1;
    else
      led_counter <= 24'd0;

    if (red_led_counter < 25'd13_499_999)  // e.g., 1s blink
      red_led_counter <= red_led_counter + 1'd1;
    else
      red_led_counter <= 25'd0;
  end
end

// LED shifting and bank2_3v3_red_led blinking logic
always @(posedge bank1_3v3_xtal_in or negedge bank3_1v8_sys_rst) begin
  if (!bank3_1v8_sys_rst) begin
    bank3_1v8_led <= 6'b111110;
    bank2_3v3_red_led <= 1'b0;
  end else begin
    // Shift the LED bits when led_counter hits max
    if (led_counter == 24'd674_9999)
      bank3_1v8_led <= {bank3_1v8_led[4:0], bank3_1v8_led[5]};

    // Toggle bank2_3v3_red_led when red_led_counter hits max
    if (red_led_counter == 25'd13_499_999)
      bank2_3v3_red_led <= ~bank2_3v3_red_led;
  end
end

endmodule

