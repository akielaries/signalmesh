/**
  * UART block component. Runs at 115200 baud assuming 27mhz clock
  */
`default_nettype none

module uart #(
  parameter BAUD_DIV = 234  // For 27 MHz clock, 115200 baud
)(
  input wire clk,
  input wire uart_rx,
  output wire uart_tx,
  output reg [5:0] led,
  input wire enable_tx,
  input wire [7:0] tx_data,
  output reg tx_done
);

// RX signals
reg [3:0] rxState = 0;
reg [12:0] rxCounter = 0;
reg [7:0] dataIn = 0;
reg [2:0] rxBitNumber = 0;
reg byteReady = 0;

localparam RX_IDLE = 0, RX_START = 1, RX_WAIT = 2, RX_READ = 3, RX_STOP = 4;
localparam HALF_BAUD = BAUD_DIV / 2;

always @(posedge clk) begin
  case (rxState)
    RX_IDLE: begin
      if (!uart_rx) begin
        rxState <= RX_START;
        rxCounter <= 1;
        rxBitNumber <= 0;
        byteReady <= 0;
      end
    end
    RX_START: begin
      if (rxCounter == HALF_BAUD) begin
        rxState <= RX_WAIT;
        rxCounter <= 1;
      end else
        rxCounter <= rxCounter + 1;
    end
    RX_WAIT: begin
      rxCounter <= rxCounter + 1;
      if (rxCounter + 1 == BAUD_DIV)
        rxState <= RX_READ;
    end
    RX_READ: begin
      rxCounter <= 1;
      dataIn <= {uart_rx, dataIn[7:1]};
      rxBitNumber <= rxBitNumber + 1;
      if (rxBitNumber == 3'b111)
        rxState <= RX_STOP;
      else
        rxState <= RX_WAIT;
    end
    RX_STOP: begin
      rxCounter <= rxCounter + 1;
      if (rxCounter + 1 == BAUD_DIV) begin
        rxState <= RX_IDLE;
        byteReady <= 1;
      end
    end
  endcase
end

always @(posedge clk) begin
  if (byteReady)
    led <= ~dataIn[5:0];
end

// TX signals
reg [3:0] txState = 0;
reg [12:0] txCounter = 0;
reg [7:0] dataOut = 0;
reg [2:0] txBitNumber = 0;
reg txReg = 1;

assign uart_tx = txReg;

localparam TX_IDLE=0, TX_START=1, TX_SEND=2, TX_STOP=3;

always @(posedge clk) begin
  case (txState)
    TX_IDLE: begin
      txReg <= 1;
      tx_done <= 0;
      if (enable_tx) begin
        txState <= TX_START;
        dataOut <= tx_data;
        txCounter <= 0;
        txBitNumber <= 0;
      end
    end
    TX_START: begin
      txReg <= 0;
      if (txCounter + 1 == BAUD_DIV) begin
        txState <= TX_SEND;
        txCounter <= 0;
      end else
        txCounter <= txCounter + 1;
    end
    TX_SEND: begin
      txReg <= dataOut[txBitNumber];
      if (txCounter + 1 == BAUD_DIV) begin
        if (txBitNumber == 3'b111) begin
          txState <= TX_STOP;
        end else
          txBitNumber <= txBitNumber + 1;
        txCounter <= 0;
      end else
        txCounter <= txCounter + 1;
    end
    TX_STOP: begin
      txReg <= 1;
      if (txCounter + 1 == BAUD_DIV) begin
        tx_done <= 1;
        txState <= TX_IDLE;
        txCounter <= 0;
      end else
        txCounter <= txCounter + 1;
    end
  endcase
end

endmodule

