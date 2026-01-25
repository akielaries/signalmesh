// uart_top.v
// Top-level module to send a "Hello, World!" string via UART
// and blink an LED on each character transmission.

module uart_top (
    // --- System Interface ---
    input  wire bank1_3v3_xtal_in,   // 27 MHz clock input (sys_clk)
    input  wire bank3_1v8_sys_rst,   // Active-low reset input

    // --- UART Interface ---
    output wire bank2_3v3_uart_tx,   // UART Transmit pin
    input  wire bank2_3v3_uart_rx,   // UART Receive pin (unused)

    // --- LED Interface ---
    output reg [5:0] bank3_1v8_led    // Onboard LEDs (6 bits, active-low)
);

    // --- Internal Signals ---
    wire sys_clk;
    wire rst_n;
    wire pll_clk_fast; // Clock from PLL

    // UART TX connections
    wire       tx_busy;
    reg        tx_start;
    reg  [7:0] tx_data;

    // --- Clock and Reset Assignment ---
    assign sys_clk = bank1_3v3_xtal_in;
    assign rst_n = bank3_1v8_sys_rst;

    // --- PLL Instantiation ---
    // Instantiate the Gowin PLL to generate a stable clock from the 27MHz crystal
    Gowin_rPLL rpll_inst (
        .clkout(pll_clk_fast), // output clock
        .clkin(sys_clk)       // input clock
    );

    // --- UART TX Instantiation ---
    uart_tx #(
        .CLOCK_FREQ(27_000_000) // Using the 27MHz clock from the crystal
    ) uart_tx_inst (
        .clk(pll_clk_fast),
        .rst_n(rst_n),
        .i_data(tx_data),
        .i_tx_start(tx_start),
        .o_tx_pin(bank2_3v3_uart_tx),
        .o_tx_busy(tx_busy)
    );

    // --- Message and Transmission Logic ---
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

    reg [$clog2(MSG_LEN)-1:0] char_index; // Index for the current character
    reg [25:0] delay_counter; // Counter for delay between full message sends
    localparam DELAY_MAX = 27_000_000; // ~1 second delay

    // Transmission state machine
    always_ff @(posedge pll_clk_fast or negedge rst_n) begin
        if (!rst_n) begin
            bank3_1v8_led <= 6'b111111; // Turn all LEDs off (active-low)
            tx_start      <= 1'b0;
            tx_data       <= 8'h00;
            char_index    <= 0;
            delay_counter <= 0;
        end else begin
            // Default: tx_start is a single-cycle pulse, so set it low
            tx_start <= 1'b0;

            if (!tx_busy) begin
                if (char_index < MSG_LEN) begin
                    // Not busy, ready to send next character
                    tx_start      <= 1'b1; // Trigger transmission
                    tx_data       <= message[char_index];
                    char_index    <= char_index + 1;
                    
                    // --- Blink LED 4 on TX ---
                    bank3_1v8_led[4] <= ~bank3_1v8_led[4];

                end else begin
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
