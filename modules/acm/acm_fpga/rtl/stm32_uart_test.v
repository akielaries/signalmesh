// stm32_uart_test: transmits a short ASCII message to the STM32 over UART once
// per second, as a first FPGA<->STM32 inter-board link bringup.
//
// message format:  "ACM:XX\r\n"  where XX is a hex byte counter that increments
// each second, so the STM32 sees a live, changing line.
//
// link: this module's UART_TX (FPGA pin 73 / IOT40A) -> STM32 UART4 RX (PD0)
//       plus a common ground. 8N1, 115200 baud.

// ---------------------------------------------------------------------------
// simple UART transmitter (8N1)
// ---------------------------------------------------------------------------
module uart_tx #(
  parameter integer CLK_FREQ = 27_000_000,
  parameter integer BAUD     = 1_000_000
) (
  input  wire       clk,
  input  wire       rst,        // active high
  input  wire [7:0] data,
  input  wire       send,       // 1-cycle strobe; ignored while busy
  output reg        tx,
  output reg        busy
);
  localparam integer DIV = CLK_FREQ / BAUD;  // clk cycles per bit

  reg [15:0] cnt;
  reg [3:0]  nbits;
  reg [9:0]  sr;   // {stop, data[7:0], start}, shifted out LSB first

  always @(posedge clk or posedge rst) begin
    if (rst) begin
      tx    <= 1'b1;
      busy  <= 1'b0;
      cnt   <= 16'd0;
      nbits <= 4'd0;
      sr    <= 10'h3FF;
    end
    else if (!busy) begin
      tx <= 1'b1;                       // idle high
      if (send) begin
        sr    <= {1'b1, data, 1'b0};    // stop bit, data, start bit (LSB)
        cnt   <= 16'd0;
        nbits <= 4'd10;                 // start + 8 data + stop
        busy  <= 1'b1;
        tx    <= 1'b0;                  // drive the start bit now
      end
    end
    else begin
      if (cnt == DIV - 1) begin
        cnt   <= 16'd0;
        tx    <= sr[1];                 // next bit out
        sr    <= {1'b1, sr[9:1]};       // shift right, fill idle
        nbits <= nbits - 1'b1;
        if (nbits == 4'd1) begin
          busy <= 1'b0;
          tx   <= 1'b1;                 // back to idle after stop bit
        end
      end
      else begin
        cnt <= cnt + 1'b1;
      end
    end
  end
endmodule

// ---------------------------------------------------------------------------
// 1 Hz message sender
// ---------------------------------------------------------------------------
module stm32_uart_test #(
  parameter integer CLK_FREQ = 27_000_000,
  parameter integer BAUD     = 1_000_000
) (
  input  wire HCLK,
  input  wire HRST,      // active high reset (S1)
  output wire UART_TX
);
  localparam integer MSG_LEN = 8;       // "ACM:XX\r\n"

  // 1 Hz tick; size the counter from CLK_FREQ so any clock frequency works
  reg [$clog2(CLK_FREQ)-1:0] sec_cnt;
  reg                        tick;
  always @(posedge HCLK or posedge HRST) begin
    if (HRST) begin
      sec_cnt <= 0;
      tick    <= 1'b0;
    end
    else if (sec_cnt == CLK_FREQ - 1) begin
      sec_cnt <= 0;
      tick    <= 1'b1;
    end
    else begin
      sec_cnt <= sec_cnt + 1'b1;
      tick    <= 1'b0;
    end
  end

  // nibble -> ascii hex
  function [7:0] hexchar(input [3:0] n);
    hexchar = (n < 4'd10) ? (8'h30 + {4'd0, n}) : (8'h41 + {4'd0, n} - 8'd10);
  endfunction

  reg  [7:0] msg_cnt;                   // counter shown as XX
  reg  [3:0] idx;
  reg        sending;
  reg  [7:0] tx_data;
  reg        tx_send;
  wire       tx_busy;

  // pick the byte for the current message index
  function [7:0] msg_byte(input [3:0] i);
    case (i)
      4'd0: msg_byte = "A";
      4'd1: msg_byte = "C";
      4'd2: msg_byte = "M";
      4'd3: msg_byte = ":";
      4'd4: msg_byte = hexchar(msg_cnt[7:4]);
      4'd5: msg_byte = hexchar(msg_cnt[3:0]);
      4'd6: msg_byte = 8'h0D;           // CR
      4'd7: msg_byte = 8'h0A;           // LF
      default: msg_byte = 8'h00;
    endcase
  endfunction

  always @(posedge HCLK or posedge HRST) begin
    if (HRST) begin
      sending <= 1'b0;
      idx     <= 4'd0;
      msg_cnt <= 8'd0;
      tx_send <= 1'b0;
      tx_data <= 8'd0;
    end
    else begin
      tx_send <= 1'b0;                  // default: no strobe
      if (!sending) begin
        if (tick) begin
          sending <= 1'b1;
          idx     <= 4'd0;
        end
      end
      else if (!tx_busy && !tx_send) begin
        tx_data <= msg_byte(idx);
        tx_send <= 1'b1;
        if (idx == MSG_LEN - 1) begin
          sending <= 1'b0;
          msg_cnt <= msg_cnt + 1'b1;
        end
        else begin
          idx <= idx + 1'b1;
        end
      end
    end
  end

  uart_tx #(
    .CLK_FREQ(CLK_FREQ),
    .BAUD(BAUD)
  ) tx_inst (
    .clk(HCLK),
    .rst(HRST),
    .data(tx_data),
    .send(tx_send),
    .tx(UART_TX),
    .busy(tx_busy)
  );

endmodule
