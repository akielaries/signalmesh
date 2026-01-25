// top.v
// Main top-level module for the project.
// - Instantiates the PLL for a stable clock.
// - Instantiates 'blink.v' to control the red LED and on-board LEDs 0 & 1.
// - Instantiates 'uart_tx.v' to send "Hello, World!".
// - Blinks on-board LED 4 for each character transmitted via UART.

module top (
    // --- System Interface ---
    input  wire bank1_3v3_xtal_in,   // 27 MHz clock input
    input  wire bank3_1v8_sys_rst,   // Active-low reset input

    // --- LED Interface ---
    output wire bank2_3v3_red_led,  // Off-board red LED
    output wire [5:0] bank3_1v8_led,    // On-board LEDs

    // --- UART Interface ---
    output wire bank2_3v3_uart_tx,   // UART Transmit pin
    input  wire bank2_3v3_uart_rx    // UART Receive pin (unused)
);

    // --- Internal Signals & Wires ---
    wire sys_clk;
    wire rst_n;
    wire pll_clk; // Main clock for the design

    // Connections for blink module
    wire red_led_from_blink;
    wire [5:0] onboard_leds_from_blink;

    // Connections for UART module
    wire tx_busy;
    reg  tx_start;
    reg  [7:0] tx_data;
    
    // State for LED 4 (TX indicator)
    reg tx_led_state;


    // --- Clocking and Reset ---
    assign sys_clk = bank1_3v3_xtal_in;
    assign rst_n = bank3_1v8_sys_rst;

    Gowin_rPLL rpll_inst (
        .clkout(pll_clk),
        .clkin(sys_clk)
    );

    // --- Module Instantiations ---

    // Instantiate the blinker module
    blink blink_inst (
        .clk(pll_clk),
        .rst_n(rst_n),
        .o_red_led(red_led_from_blink),
        .o_onboard_leds(onboard_leds_from_blink)
    );

    // Instantiate the UART transmitter
    uart_tx #(
        .CLOCK_FREQ(27_000_000)
    ) uart_tx_inst (
        .clk(pll_clk),
        .rst_n(rst_n),
        .i_data(tx_data),
        .i_tx_start(tx_start),
        .o_tx_pin(bank2_3v3_uart_tx),
        .o_tx_busy(tx_busy)
    );


    // --- UART Control Logic ---
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

    // Control FSM for sending UART message and controlling tx_led_state
    always @(posedge pll_clk or negedge rst_n) begin
        if (!rst_n) begin
            tx_start      <= 1'b0;
            tx_data       <= 8'h00;
            char_index    <= 0;
            delay_counter <= 0;
            tx_led_state  <= 1'b1; // LED 4 OFF initially (active-low)
        end else begin
            // Default: tx_start is a single-cycle pulse, so always set it low
            tx_start <= 1'b0;

            if (!tx_busy) begin
                if (char_index < MSG_LEN) begin
                    // Ready to send next character
                    tx_start      <= 1'b1;
                    tx_data       <= message[char_index];
                    char_index    <= char_index + 1;
                    
                    // Toggle LED 4 on TX
                    tx_led_state <= ~tx_led_state;
                end else begin // Corrected 'else {' to 'else begin'
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
    
    // --- Final Output Assignments ---
    assign bank2_3v3_red_led = red_led_from_blink; // Off-board red LED from blink module

    // Combine onboard LED outputs from blink module and local TX indicator
    assign bank3_1v8_led = {
        1'b1, // LED 5 always off (active-low)
        tx_led_state, // LED 4 controlled by TX indicator (active-low)
        1'b1, // LED 3 always off (active-low)
        1'b1, // LED 2 always off (active-low)
        onboard_leds_from_blink[1], // LED 1 from blink module (active-low)
        onboard_leds_from_blink[0]  // LED 0 from blink module (active-low)
    };

endmodule