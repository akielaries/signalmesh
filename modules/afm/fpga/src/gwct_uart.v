// =============================================================================
// gwct_uart.v — 8N1 UART, 50 MHz clock, 115200 baud
//
// Ports:
//   clk, rstn          — system clock and active-low reset
//   rx                 — serial receive line  (GWCT_RX)
//   tx                 — serial transmit line (GWCT_TX)
//
//   rx_data [7:0]      — received byte
//   rx_valid           — pulses high for 1 clock when rx_data is valid
//
//   tx_data [7:0]      — byte to transmit
//   tx_valid           — assert for 1 clock to start transmission
//   tx_ready           — high when transmitter is idle and can accept a byte
// =============================================================================

module gwct_uart #(
    parameter CLK_HZ  = 50_000_000,
    parameter BAUD    = 115_200
)(
    input            clk,
    input            rstn,

    // physical pins
    input            rx,
    output reg       tx,

    // receive side
    output reg [7:0] rx_data,
    output reg       rx_valid,

    // transmit side
    input      [7:0] tx_data,
    input            tx_valid,
    output           tx_ready
);

    // -------------------------------------------------------------------------
    // Baud rate divisor
    // -------------------------------------------------------------------------
    localparam integer DIVISOR = CLK_HZ / BAUD;          // 434 at 50MHz/115200
    localparam integer HALF    = DIVISOR / 2;

    // =========================================================================
    // RX
    // =========================================================================
    // synchronise rx to avoid metastability
    reg rx_s0, rx_s1, rx_s2;
    always @(posedge clk) begin
        rx_s0 <= rx;
        rx_s1 <= rx_s0;
        rx_s2 <= rx_s1;
    end
    wire rx_in = rx_s2;

    // FSM
    localparam RX_IDLE  = 2'd0,
               RX_START = 2'd1,
               RX_DATA  = 2'd2,
               RX_STOP  = 2'd3;

    reg [1:0]  rx_state;
    reg [15:0] rx_cnt;      // baud counter
    reg [2:0]  rx_bit;      // bit index 0..7
    reg [7:0]  rx_shift;

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            rx_state <= RX_IDLE;
            rx_cnt   <= 0;
            rx_bit   <= 0;
            rx_shift <= 0;
            rx_data  <= 0;
            rx_valid <= 0;
        end else begin
            rx_valid <= 0;  // default: not valid

            case (rx_state)

                RX_IDLE: begin
                    if (!rx_in) begin           // falling edge = start bit
                        rx_state <= RX_START;
                        rx_cnt   <= HALF;       // sample in the middle of start bit
                    end
                end

                RX_START: begin
                    if (rx_cnt == 0) begin
                        rx_cnt   <= DIVISOR - 1;
                        rx_state <= RX_DATA;
                        rx_bit   <= 0;
                    end else begin
                        rx_cnt <= rx_cnt - 1;
                    end
                end

                RX_DATA: begin
                    if (rx_cnt == 0) begin
                        rx_shift <= {rx_in, rx_shift[7:1]};  // LSB first
                        rx_cnt   <= DIVISOR - 1;
                        if (rx_bit == 7) begin
                            rx_state <= RX_STOP;
                        end else begin
                            rx_bit <= rx_bit + 1;
                        end
                    end else begin
                        rx_cnt <= rx_cnt - 1;
                    end
                end

                RX_STOP: begin
                    if (rx_cnt == 0) begin
                        // stop bit — if line is high, frame is good
                        if (rx_in) begin
                            rx_data  <= rx_shift;
                            rx_valid <= 1;
                        end
                        rx_state <= RX_IDLE;
                    end else begin
                        rx_cnt <= rx_cnt - 1;
                    end
                end

            endcase
        end
    end

    // =========================================================================
    // TX
    // =========================================================================
    localparam TX_IDLE  = 2'd0,
               TX_START = 2'd1,
               TX_DATA  = 2'd2,
               TX_STOP  = 2'd3;

    reg [1:0]  tx_state;
    reg [15:0] tx_cnt;
    reg [2:0]  tx_bit;
    reg [7:0]  tx_shift;
    reg        tx_busy;

    assign tx_ready = (tx_state == TX_IDLE);

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            tx_state <= TX_IDLE;
            tx_cnt   <= 0;
            tx_bit   <= 0;
            tx_shift <= 0;
            tx       <= 1'b1;   // idle high
        end else begin

            case (tx_state)

                TX_IDLE: begin
                    tx <= 1'b1;
                    if (tx_valid) begin
                        tx_shift <= tx_data;
                        tx_state <= TX_START;
                        tx_cnt   <= DIVISOR - 1;
                        tx       <= 1'b0;       // start bit
                    end
                end

                TX_START: begin
                    if (tx_cnt == 0) begin
                        tx       <= tx_shift[0];
                        tx_shift <= {1'b1, tx_shift[7:1]};
                        tx_bit   <= 0;
                        tx_cnt   <= DIVISOR - 1;
                        tx_state <= TX_DATA;
                    end else begin
                        tx_cnt <= tx_cnt - 1;
                    end
                end

                TX_DATA: begin
                    if (tx_cnt == 0) begin
                        if (tx_bit == 7) begin
                            tx       <= 1'b1;   // stop bit
                            tx_state <= TX_STOP;
                            tx_cnt   <= DIVISOR - 1;
                        end else begin
                            tx_bit   <= tx_bit + 1;
                            tx       <= tx_shift[0];
                            tx_shift <= {1'b1, tx_shift[7:1]};
                            tx_cnt   <= DIVISOR - 1;
                        end
                    end else begin
                        tx_cnt <= tx_cnt - 1;
                    end
                end

                TX_STOP: begin
                    if (tx_cnt == 0) begin
                        tx_state <= TX_IDLE;
                    end else begin
                        tx_cnt <= tx_cnt - 1;
                    end
                end

            endcase
        end
    end

endmodule