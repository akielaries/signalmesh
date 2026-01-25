module pll_blink (
  // --- Interface ---
  input  bank1_3v3_xtal_in,   // 27 MHz clock input (sys_clk)
  input  bank3_1v8_sys_rst,   // active-low reset input
  output bank2_3v3_red_led, // Off-board red LED (active-high)
  output reg [5:0] bank3_1v8_led  // Onboard LEDs (6 bits, active-low)
);

  // --- Internal Signals ---
  wire sys_clk;
  wire rst_n;
  wire pll_clk_fast;

  assign sys_clk = bank1_3v3_xtal_in;
  assign rst_n = bank3_1v8_sys_rst;

  Gowin_rPLL rpll_inst(
    .clkout(pll_clk_fast),
    .clkin(sys_clk)
  );

  localparam RED_LED_BLINK_DIV = 2_333_333;
  reg [25:0] red_led_counter;
  reg red_led_toggle_en; // New signal

  always @(posedge pll_clk_fast or negedge rst_n) begin
    if (!rst_n) begin
      red_led_counter <= 0;
      red_led_toggle_en <= 1'b0;
    end
    else begin
      if (red_led_counter == RED_LED_BLINK_DIV - 1) begin
        red_led_counter <= 0;
        red_led_toggle_en <= 1'b1; // Pulse for one clock cycle
      end
      else begin
        red_led_counter <= red_led_counter + 1;
        red_led_toggle_en <= 1'b0; // Clear the pulse
      end
    end
  end

  reg red_led_state_q;

  always @(posedge pll_clk_fast or negedge rst_n) begin
    if (!rst_n) begin
      red_led_state_q <= 1'b0;
    end else if (red_led_toggle_en) begin
      red_led_state_q <= ~red_led_state_q;
    end
  end

  assign bank2_3v3_red_led = pll_clk_fast ? red_led_state_q : ~red_led_state_q;



  localparam ONBOARD_LED0_BLINK_DIV = 10_500_000;
  reg [23:0] onboard_led0_counter;

  localparam ONBOARD_LED1_BLINK_DIV = 21_000_000;
  reg [24:0] onboard_led1_counter;

  always @(posedge pll_clk_fast or negedge rst_n) begin
    if (!rst_n) begin
      onboard_led0_counter <= 0;
      onboard_led1_counter <= 0;
      bank3_1v8_led <= 6'b111111; // All OFF (active-low)
    end
    else begin
      // LED 0
      if (onboard_led0_counter == ONBOARD_LED0_BLINK_DIV - 1) begin
        onboard_led0_counter <= 0;
        bank3_1v8_led[0] <= ~bank3_1v8_led[0]; // Toggle
      end
      else begin
        onboard_led0_counter <= onboard_led0_counter + 1;
      end

      // LED 1
      if (onboard_led1_counter == ONBOARD_LED1_BLINK_DIV - 1) begin
        onboard_led1_counter <= 0;
        bank3_1v8_led[1] <= ~bank3_1v8_led[1]; // Toggle
      end
      else begin
        onboard_led1_counter <= onboard_led1_counter + 1;
      end

    end
  end

endmodule
