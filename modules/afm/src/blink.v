// blink.v
// Reusable module to blink one off-board LED and two on-board LEDs.

module blink (
    // --- Interface ---
    input  wire clk,   // Clock input (from a PLL)
    input  wire rst_n, // Active-low reset

    // --- Outputs ---
    output wire o_red_led,      // Off-board red LED output
    output reg [5:0] o_onboard_leds // On-board LEDs output
);

    // --- Red LED Blinker ---
    localparam RED_LED_BLINK_DIV = 4666666; // ~0.5 second toggle at 27MHz
    reg [23:0] red_led_counter;
    reg red_led_state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            red_led_counter <= 0;
            red_led_state   <= 1'b0;
        end else begin
            if (red_led_counter == RED_LED_BLINK_DIV - 1) begin
                red_led_counter <= 0;
                red_led_state   <= ~red_led_state; // Toggle
            end else begin
                red_led_counter <= red_led_counter + 1;
            end
        end
    end
    assign o_red_led = red_led_state;


    // --- On-board LED Blinker ---
    localparam ONBOARD_LED0_BLINK_DIV = 10_500_000;
    reg [23:0] onboard_led0_counter;

    localparam ONBOARD_LED1_BLINK_DIV = 21_000_000;
    reg [24:0] onboard_led1_counter;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            onboard_led0_counter <= 0;
            onboard_led1_counter <= 0;
            o_onboard_leds       <= 6'b111111; // All OFF (active-low)
        end else begin
            // LED 0
            if (onboard_led0_counter == ONBOARD_LED0_BLINK_DIV - 1) begin
                onboard_led0_counter   <= 0;
                o_onboard_leds[0] <= ~o_onboard_leds[0]; // Toggle
            end else begin
                onboard_led0_counter <= onboard_led0_counter + 1;
            end

            // LED 1
            if (onboard_led1_counter == ONBOARD_LED1_BLINK_DIV - 1) begin
                onboard_led1_counter   <= 0;
                o_onboard_leds[1] <= ~o_onboard_leds[1]; // Toggle
            end else begin
                onboard_led1_counter <= onboard_led1_counter + 1;
            end
        end
    end

endmodule