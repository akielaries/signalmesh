// uart_tx.v
// Generic UART Transmitter Module

module uart_tx #(
    parameter CLOCK_FREQ = 27_000_000, // System clock frequency in Hz
    parameter BAUD_RATE  = 115200       // Desired baud rate
) (
    input wire        clk,         // System clock
    input wire        rst_n,       // Asynchronous active-low reset
    input wire [7:0]  i_data,      // 8-bit data to transmit
    input wire        i_tx_start,  // High for one clock cycle to start transmission

    output reg        o_tx_pin,    // UART Transmit pin (active-low start bit, active-high stop bit)
    output reg        o_tx_busy    // High when transmission is in progress
);

    // Baud rate generator
    // CLK_PER_BIT: Number of clock cycles per UART bit period.
    localparam CLK_PER_BIT = CLOCK_FREQ / BAUD_RATE;

    // State machine for UART transmission
    localparam IDLE      = 2'b00;
    localparam START_BIT = 2'b01;
    localparam DATA_BITS = 2'b10;
    localparam STOP_BIT  = 2'b11;

    reg [1:0] state;
    reg [3:0] bit_index; // 0-7 for data bits
    reg [$clog2(CLK_PER_BIT)-1:0] clk_counter;
    reg [7:0] tx_data_reg;

    // FSM for UART transmission
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            o_tx_pin <= 1'b1; // Idle high
            o_tx_busy <= 1'b0;
            clk_counter <= 0;
            bit_index <= 0;
            tx_data_reg <= 8'h00;
        end else begin
            case (state)
                IDLE: begin
                    o_tx_pin <= 1'b1;
                    o_tx_busy <= 1'b0;
                    clk_counter <= 0;
                    bit_index <= 0;
                    if (i_tx_start) begin
                        tx_data_reg <= i_data;
                        o_tx_busy <= 1'b1;
                        state <= START_BIT;
                    end
                end

                START_BIT: begin
                    o_tx_pin <= 1'b0; // Start bit is low
                    if (clk_counter == CLK_PER_BIT - 1) begin
                        clk_counter <= 0;
                        state <= DATA_BITS;
                    end else begin
                        clk_counter <= clk_counter + 1;
                    end
                end

                DATA_BITS: begin
                    o_tx_pin <= tx_data_reg[bit_index];
                    if (clk_counter == CLK_PER_BIT - 1) begin
                        clk_counter <= 0;
                        if (bit_index == 7) begin
                            state <= STOP_BIT;
                        end else begin
                            bit_index <= bit_index + 1;
                        end
                    end else begin
                        clk_counter <= clk_counter + 1;
                    end
                end

                STOP_BIT: begin
                    o_tx_pin <= 1'b1; // Stop bit is high
                    if (clk_counter == CLK_PER_BIT - 1) begin
                        clk_counter <= 0;
                        state <= IDLE;
                    end else begin
                        clk_counter <= clk_counter + 1;
                    end
                end

                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule