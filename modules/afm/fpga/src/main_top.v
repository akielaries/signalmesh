// main_top.v
// Top-level module that combines the red LED blinker and the UART "Hello, World!" sender.
// - Blinks the off-board red LED.
// - Sends "Hello, World!\n" repeatedly via UART.
// - Blinks on-board LED 4 for each character transmitted.

module main_top (
    // --- System Interface ---
    input  wire bank1_3v3_xtal_in,   // 27 MHz clock input (sys_clk)
    input  wire bank3_1v8_sys_rst,   // Active-low reset input

    // --- LED Interface ---
    output wire      bank2_3v3_red_led,  // Off-board red LED (active-high)
    output reg [5:0] bank3_1v8_led,    // Onboard LEDs (6 bits, active-low)

    // --- UART Interface ---
    output wire bank2_3v3_uart_tx,   // UART Transmit pin
    input  wire bank2_3v3_uart_rx    // UART Receive pin (unused)
);

    // --- Internal Signals ---
    wire sys_clk;
    wire rst_n;
    wire pll_clk_fast; // Clock from PLL

    // --- Clock and Reset Assignment ---
    assign sys_clk = bank1_3v3_xtal_in;
    assign rst_n = bank3_1v8_sys_rst;

    // --- PLL Instantiation ---
    Gowin_rPLL rpll_inst (
        .clkout(pll_clk_fast),
        .clkin(sys_clk)
    );


    // =========================================================================
    // Red LED Blinker Logic (from pll_blink.v)
    // =========================================================================
    localparam RED_LED_BLINK_DIV = 2_333_333;
    reg [25:0] red_led_counter;
    reg red_led_toggle_en; // Pulse to enable the DDR-style output

    always @(posedge pll_clk_fast or negedge rst_n) begin
        if (!rst_n) begin
            red_led_counter <= 0;
            red_led_toggle_en <= 1'b0;
        end else begin
            if (red_led_counter == RED_LED_BLINK_DIV - 1) begin
                red_led_counter <= 0;
                red_led_toggle_en <= 1'b1; // Pulse for one clock cycle
            end else begin
                red_led_counter <= red_led_counter + 1;
                red_led_toggle_en <= 1'b0; // Clear the pulse
            end
        end
    end

    // The red LED will be ON during the high phase of the clock when enabled
    assign bank2_3v3_red_led = red_led_toggle_en & pll_clk_fast;


    // =========================================================================
    // UART Sender Logic (from uart_top.v)
    // =========================================================================
    wire       tx_busy;
    reg        tx_start;
    reg  [7:0] tx_data;

    uart_tx #(
        .CLOCK_FREQ(27_000_000)
    ) uart_tx_inst (
        .clk(pll_clk_fast),
        .rst_n(rst_n),
        .i_data(tx_data),
        .i_tx_start(tx_start),
        .o_tx_pin(bank2_3v3_uart_tx),
        .o_tx_busy(tx_busy)
    );

    localparam MSG_LEN = 14;
    reg [7:0] message [0:MSG_LEN-1];
    initial begin
        message[0]  = "H";
        message[1]  = "e";
        message[2]  = "l";
        message[3]  = "l";
        message[4]  = "o";
        message[5]  = ",";
        message[6]  = " ";
        message[7]  = "W";
        message[8]  = "o";
        message[9]  = "r";
        message[10] = "l";
        message[11] = "d";
        message[12] = "!";
        message[13] = "\n";
    end

    reg [$clog2(MSG_LEN)-1:0] char_index;
    reg [25:0] delay_counter;
    localparam DELAY_MAX = 27_000_000; // ~1 second delay

    // Combined state machine for UART sending and on-board LED control
    always @(posedge pll_clk_fast or negedge rst_n) begin
        if (!rst_n) begin
            bank3_1v8_led <= 6'b111111; // Turn all on-board LEDs off
            tx_start      <= 1'b0;
            tx_data       <= 8'h00;
            char_index    <= 0;
            delay_counter <= 0;
        end else begin
            // Default: tx_start is a single-cycle pulse, so always set it low
            tx_start <= 1'b0;

            if (!tx_busy) begin
                if (char_index < MSG_LEN) begin
                    // Ready to send next character
                    tx_start      <= 1'b1;
                    tx_data       <= message[char_index];
                    char_index    <= char_index + 1;
                    
                    // Blink LED 4 on TX
                    bank3_1v8_led[4] <= ~bank3_1v8_led[4];

                end else {
                    // End of message, start delay
                    if (delay_counter < DELAY_MAX) begin
                        delay_counter <= delay_counter + 1;
                    end else begin
                        delay_counter <= 0;
                        char_index    <= 0; // Reset to start of message
                    end
                end
            end
        end
    end

endmodule
