// uart.v
// Combined UART Transceiver (TX and RX) module
// Implements "Hello, World!" TX and simple RX, controlling LEDs.

module uart
#(
    parameter DELAY_FRAMES = 234 // 27,000,000 (27Mhz) / 115200 Baud rate
)(
    input clk,
    input uart_rx,
    output reg uart_tx, // Corrected: output reg
    output reg [5:0] led,
    input btn1 // Used as reset for now
);

    // --- TX State machine for UART transmission ---
    localparam TX_IDLE      = 2'b00;
    localparam TX_START_BIT = 2'b01;
    localparam TX_DATA_BITS = 2'b10;
    localparam TX_STOP_BIT  = 2'b11;

    reg [1:0] tx_state;
    reg [3:0] tx_bit_index; // 0-7 for data bits
    reg [$clog2(DELAY_FRAMES)-1:0] tx_clk_counter;
    reg [7:0] tx_data_reg;
    reg current_tx_bit; // Holds the bit currently being transmitted
    reg tx_busy; // Internal tx_busy signal


    // --- RX State machine for UART reception ---
    localparam RX_IDLE         = 3'b000;
    localparam RX_START_BIT    = 3'b001;
    localparam RX_DATA_BITS    = 3'b010;
    localparam RX_STOP_BIT     = 3'b011;

    reg [2:0] rx_state;
    reg [$clog2(DELAY_FRAMES)-1:0] rx_clk_counter;
    reg [3:0] rx_bit_index;
    reg [7:0] rx_buffer;
    reg rx_pin_sync; // Synchronized version of i_uart_rx
    reg rx_data_valid; // Pulse when new RX data is available
    reg [3:0] next_bit_index; // Declared at top of always block, size adjusted if needed (4 bits for 0-8)
    
    // --- "Hello, World!" Message and TX Control ---
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
    reg [24:0] delay_counter; // Corrected bit width for 27_000_000
    localparam DELAY_MAX = 25'd27_000_000; // Explicitly sized ~1 second delay for 27MHz clock


    // --- Synchronize RX input (avoids metastability) ---
    always @(posedge clk) begin
        rx_pin_sync <= uart_rx;
    end

    // --- Main UART FSM (TX and RX) ---
    always @(posedge clk or negedge btn1) begin // btn1 as active-low reset
        if (!btn1) begin
            // TX Reset
            tx_state <= TX_IDLE;
            uart_tx <= 1'b1; // Idle high
            tx_busy <= 1'b0;
            tx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
            tx_bit_index <= 4'd0; // Explicitly sized 0
            tx_data_reg <= 8'h00;
            current_tx_bit <= 1'b1; // Initialize to idle high

            // RX Reset
            rx_state <= RX_IDLE;
            rx_data_valid <= 1'b0;
            rx_buffer <= 8'h00;
            rx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
            rx_bit_index <= 4'd0; // Explicitly sized 0

            // Message TX Reset
            char_index <= {($clog2(MSG_LEN)){1'b0}}; // Explicitly sized 0
            delay_counter <= 25'd0; // Explicitly sized 0

            // LED Reset
            led <= 6'b000000; // All LEDs off
        end else begin
            // Default: rx_data_valid is a pulse
            rx_data_valid <= 1'b0;

            // --- TX Logic ---
            case (tx_state)
                TX_IDLE: begin
                    tx_busy <= 1'b0;
                    current_tx_bit <= 1'b1; // Ensure idle high
                    tx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                    tx_bit_index <= 4'd0; // Explicitly sized 0

                    if (!tx_busy && char_index < MSG_LEN && delay_counter == 25'd0) begin // Only send if not busy, have chars, and no delay
                        tx_data_reg <= message[char_index];
                        tx_busy <= 1'b1;
                        tx_state <= TX_START_BIT;
                        current_tx_bit <= 1'b0; // Start bit is low
                        
                        // Increment char_index and toggle LED4 here to avoid separate FSM
                        char_index <= char_index + 4'd1; // Explicitly sized 1
                        led[4] <= ~led[4]; // Toggle LED4 on each TX_START
                    end else if (char_index == MSG_LEN) begin // End of message, start delay
                        delay_counter <= delay_counter + 25'd1; // Explicitly sized 1
                        if (delay_counter == DELAY_MAX - 25'd1) begin // Explicitly sized 1
                            delay_counter <= 25'd0; // Explicitly sized 0
                            char_index <= {($clog2(MSG_LEN)){1'b0}}; // Explicitly sized 0 // Reset to start of message after delay
                        end
                    end
                end

                TX_START_BIT: begin
                    if (tx_clk_counter == DELAY_FRAMES - 4'd1) begin // Explicitly sized 1
                        tx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                        tx_state <= TX_DATA_BITS;
                        tx_bit_index <= 4'd0; // Explicitly sized 0
                        current_tx_bit <= tx_data_reg[0]; // First data bit (LSB)
                    end else begin
                        tx_clk_counter <= tx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end

                TX_DATA_BITS: begin
                    if (tx_clk_counter == DELAY_FRAMES - 4'd1) begin // Explicitly sized 1
                        tx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                        if (tx_bit_index == 4'd7) begin // Last data bit sent (index 7) Explicitly sized 7
                            tx_state <= TX_STOP_BIT;
                            current_tx_bit <= 1'b1; // Stop bit is high
                        end else begin
                            next_bit_index = tx_bit_index + 4'd1; // Explicitly sized 1
                            tx_bit_index <= next_bit_index;
                            current_tx_bit <= tx_data_reg[next_bit_index]; // Next data bit
                        end
                    end else begin
                        tx_clk_counter <= tx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end

                TX_STOP_BIT: begin
                    if (tx_clk_counter == DELAY_FRAMES - 4'd1) begin // Explicitly sized 1
                        tx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                        tx_state <= TX_IDLE;
                        current_tx_bit <= 1'b1; // Return to idle high
                    end else begin
                        tx_clk_counter <= tx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end

                default: begin
                    tx_state <= TX_IDLE;
                    current_tx_bit <= 1'b1;
                end
            endcase
            uart_tx <= current_tx_bit; // Drive the output pin from the stable register


            // --- RX Logic ---
            case (rx_state)
                RX_IDLE: begin
                    rx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                    rx_bit_index <= 4'd0; // Explicitly sized 0
                    if (!rx_pin_sync) begin // Start bit detected (low)
                        rx_state <= RX_START_BIT;
                    end
                end
                RX_START_BIT: begin
                    if (rx_clk_counter == DELAY_FRAMES + (DELAY_FRAMES/4'd2) - 4'd1) begin // Sample in middle of start bit
                        if (!rx_pin_sync) begin // Still low, valid start bit
                            rx_state <= RX_DATA_BITS;
                            rx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0 // Reset for first data bit
                            rx_bit_index <= 4'd0; // Explicitly sized 0
                        end else begin // Glitch or false start bit
                            rx_state <= RX_IDLE;
                        end
                    end else begin
                        rx_clk_counter <= rx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end
                RX_DATA_BITS: begin
                    if (rx_clk_counter == DELAY_FRAMES - 4'd1) begin // Sample middle of data bit
                        rx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                        rx_buffer[rx_bit_index] <= rx_pin_sync; // Store bit (LSB first)

                        if (rx_bit_index == 4'd7) begin // Last data bit received Explicitly sized 7
                            rx_state <= RX_STOP_BIT;
                        end else begin
                            rx_bit_index <= rx_bit_index + 4'd1; // Explicitly sized 1
                        end
                    end else begin
                        rx_clk_counter <= rx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end
                RX_STOP_BIT: begin
                    if (rx_clk_counter == DELAY_FRAMES - 4'd1) begin // Sample middle of stop bit
                        if (rx_pin_sync) begin // Stop bit should be high
                            rx_data_valid <= 1'b1; // Pulse data valid
                            led[0] <= ~led[0]; // Toggle LED0 on RX_VALID
                            led[1] <= rx_buffer[0]; // Display LSB of received data
                            led[2] <= rx_buffer[1];
                            led[3] <= rx_buffer[2];
                        end else begin // Framing error (stop bit low)
                            led[5] <= ~led[5]; // Toggle LED5 on RX_ERROR
                        end
                        rx_state <= RX_IDLE; // Return to IDLE
                        rx_clk_counter <= {($clog2(DELAY_FRAMES)){1'b0}}; // Explicitly sized 0
                    end else begin
                        rx_clk_counter <= rx_clk_counter + {($clog2(DELAY_FRAMES)){1'b0}} + 4'd1; // Explicitly sized 1
                    end
                end
                default: rx_state <= RX_IDLE;
            endcase
        end
    end

endmodule