// =============================================================================
// gwct_packet.v — packet framing layer for GWCT debug protocol
//
// RX packet (host → FPGA), 11 bytes:
//   [0]     0x47        magic ('G')
//   [1]     CMD         0x01=read, 0x02=write
//   [2..5]  ADDR[31:0]  little-endian
//   [6..9]  DATA[31:0]  little-endian
//   [10]    CHK         XOR of bytes [0..9]
//
// TX response (FPGA → host), 7 bytes:
//   [0]     0x47        magic
//   [1]     CMD echo    (0xFF on error)
//   [2..5]  DATA[31:0]  little-endian
//   [6]     CHK         XOR of bytes [0..5]
// =============================================================================

module gwct_packet (
    input            clk,
    input            rstn,

    // UART byte interface
    input      [7:0] rx_data,
    input            rx_valid,
    output reg [7:0] tx_data,
    output reg       tx_valid,
    input            tx_ready,

    // APB master command interface
    output reg [31:0] cmd_addr,
    output reg [31:0] cmd_wdata,
    output reg        cmd_write,
    output reg        cmd_valid,
    input             cmd_ready,
    input      [31:0] cmd_rdata,
    input             cmd_error
);

    // =========================================================================
    // RX state machine
    // =========================================================================
    localparam RX_MAGIC  = 4'd0,
               RX_CMD    = 4'd1,
               RX_ADDR0  = 4'd2,
               RX_ADDR1  = 4'd3,
               RX_ADDR2  = 4'd4,
               RX_ADDR3  = 4'd5,
               RX_DATA0  = 4'd6,
               RX_DATA1  = 4'd7,
               RX_DATA2  = 4'd8,
               RX_DATA3  = 4'd9,
               RX_CHK    = 4'd10,
               RX_WAIT   = 4'd11;

    reg [3:0]  rx_state;
    reg [7:0]  rx_cmd;
    reg [31:0] rx_addr;
    reg [31:0] rx_wdata;
    reg [7:0]  rx_chk_acc;

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            rx_state   <= RX_MAGIC;
            rx_cmd     <= 0;
            rx_addr    <= 0;
            rx_wdata   <= 0;
            rx_chk_acc <= 0;
            cmd_addr   <= 0;
            cmd_wdata  <= 0;
            cmd_write  <= 0;
            cmd_valid  <= 0;
        end else begin
            cmd_valid <= 0;

            if (rx_state == RX_WAIT) begin
                if (tx_done)
                    rx_state <= RX_MAGIC;
            end else if (rx_valid) begin
                case (rx_state)

                    RX_MAGIC: begin
                        if (rx_data == 8'h47) begin
                            rx_chk_acc <= 8'h47;
                            rx_state   <= RX_CMD;
                        end
                    end

                    RX_CMD: begin
                        rx_cmd     <= rx_data;
                        rx_chk_acc <= rx_chk_acc ^ rx_data;
                        if (rx_data == 8'h01 || rx_data == 8'h02)
                            rx_state <= RX_ADDR0;
                        else
                            rx_state <= RX_MAGIC;
                    end

                    RX_ADDR0: begin rx_addr[ 7: 0] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_ADDR1; end
                    RX_ADDR1: begin rx_addr[15: 8] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_ADDR2; end
                    RX_ADDR2: begin rx_addr[23:16] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_ADDR3; end
                    RX_ADDR3: begin rx_addr[31:24] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_DATA0; end

                    RX_DATA0: begin rx_wdata[ 7: 0] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_DATA1; end
                    RX_DATA1: begin rx_wdata[15: 8] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_DATA2; end
                    RX_DATA2: begin rx_wdata[23:16] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_DATA3; end
                    RX_DATA3: begin rx_wdata[31:24] <= rx_data; rx_chk_acc <= rx_chk_acc ^ rx_data; rx_state <= RX_CHK;   end

                    RX_CHK: begin
                        if ((rx_chk_acc ^ rx_data) == 8'h00) begin
                            cmd_addr  <= rx_addr;
                            cmd_wdata <= rx_wdata;
                            cmd_write <= (rx_cmd == 8'h02);
                            cmd_valid <= 1;
                            rx_state  <= RX_WAIT;
                        end else begin
                            rx_state <= RX_MAGIC;
                        end
                    end

                    default: rx_state <= RX_MAGIC;
                endcase
            end
        end
    end

    // =========================================================================
    // TX — 7-byte response buffer, drained one byte per tx_ready window.
    //
    // Latch cmd_rdata and cmd_error when cmd_ready pulses (TX_IDLE→TX_LOAD),
    // because by the time we're draining bytes those inputs may have changed.
    // =========================================================================
    localparam TX_IDLE = 3'd0,
               TX_LOAD = 3'd1,
               TX_SEND = 3'd2,
               TX_WAIT = 3'd3,   // wait for UART to go busy (tx_ready low)
               TX_DONE = 3'd4;

    reg [2:0]  tx_state;
    reg [2:0]  tx_idx;
    reg        tx_done;

    // Response buffer — module-level regs, no scoping issues
    reg [7:0]  tx_buf0;
    reg [7:0]  tx_buf1;
    reg [7:0]  tx_buf2;
    reg [7:0]  tx_buf3;
    reg [7:0]  tx_buf4;
    reg [7:0]  tx_buf5;
    reg [7:0]  tx_buf6;

    // Latch of APB result (held stable for entire TX sequence)
    reg [31:0] tx_rdata;
    reg        tx_error;
    reg [7:0]  tx_cmd;

    integer i;

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            tx_state <= TX_IDLE;
            tx_valid <= 0;
            tx_data  <= 0;
            tx_idx   <= 0;
            tx_done  <= 0;
            tx_rdata <= 0;
            tx_error <= 0;
            tx_cmd   <= 0;
            tx_buf0  <= 0; tx_buf1 <= 0; tx_buf2 <= 0; tx_buf3 <= 0;
            tx_buf4  <= 0; tx_buf5 <= 0; tx_buf6 <= 0;
        end else begin
            tx_valid <= 0;
            tx_done  <= 0;

            case (tx_state)

                TX_IDLE: begin
                    if (cmd_ready) begin
                        // Latch results NOW while they're valid
                        tx_rdata <= cmd_rdata;
                        tx_error <= cmd_error;
                        tx_cmd   <= rx_cmd;
                        tx_state <= TX_LOAD;
                    end
                end

                TX_LOAD: begin
                    // All expressions use latched regs no scoping needed
                    tx_buf0 <= 8'h47;
                    tx_buf1 <= tx_error ? 8'hFF : tx_cmd;
                    tx_buf2 <= tx_error ? 8'h00 : tx_rdata[ 7: 0];
                    tx_buf3 <= tx_error ? 8'h00 : tx_rdata[15: 8];
                    tx_buf4 <= tx_error ? 8'h00 : tx_rdata[23:16];
                    tx_buf5 <= tx_error ? 8'h00 : tx_rdata[31:24];
                    // checksum = XOR of buf[0..5], computed from same latched values
                    tx_buf6 <= 8'h47
                               ^ (tx_error ? 8'hFF : tx_cmd)
                               ^ (tx_error ? 8'h00 : tx_rdata[ 7: 0])
                               ^ (tx_error ? 8'h00 : tx_rdata[15: 8])
                               ^ (tx_error ? 8'h00 : tx_rdata[23:16])
                               ^ (tx_error ? 8'h00 : tx_rdata[31:24]);
                    tx_idx   <= 0;
                    tx_state <= TX_SEND;
                end

                TX_SEND: begin
                    if (tx_ready) begin
                        case (tx_idx)
                            3'd0: tx_data <= tx_buf0;
                            3'd1: tx_data <= tx_buf1;
                            3'd2: tx_data <= tx_buf2;
                            3'd3: tx_data <= tx_buf3;
                            3'd4: tx_data <= tx_buf4;
                            3'd5: tx_data <= tx_buf5;
                            3'd6: tx_data <= tx_buf6;
                            default: tx_data <= 8'h00;
                        endcase
                        tx_valid <= 1;
                        tx_state <= TX_WAIT;    // wait for UART to go busy
                    end
                end

                TX_WAIT: begin
                    // tx_ready goes low the cycle after tx_valid was asserted.
                    // Wait here until it does, then either send next byte or finish.
                    if (!tx_ready) begin
                        if (tx_idx == 3'd6)
                            tx_state <= TX_DONE;
                        else begin
                            tx_idx   <= tx_idx + 3'd1;
                            tx_state <= TX_SEND;    // go back, wait for tx_ready high
                        end
                    end
                end

                TX_DONE: begin
                    tx_done  <= 1;
                    tx_state <= TX_IDLE;
                end

            endcase
        end
    end

endmodule