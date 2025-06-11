`default_nettype none

module uart_test(
  input wire clk,
  output wire uart_tx,
  output wire [5:0] led
);

reg [7:0] test_data [0:15];
initial begin
  test_data[0] = "M";
  test_data[1] = "I";
  test_data[2] = "S";
  test_data[3] = "T";
  test_data[4] = "Y";
  test_data[5] = "S";
  test_data[6] = "T";
  test_data[7] = "I";
  test_data[8] = "N";
  test_data[9] = "K";
  test_data[10] = "S";
  test_data[11] = "!";
  test_data[12] = "\r";
  test_data[13] = "\n";
  test_data[14] = "\r";
  test_data[15] = "\n";
end

reg [3:0] idx = 0;
reg enable_tx = 0;
wire tx_done;

uart #(234) u(
  .clk(clk),
  .uart_rx(1'b1),  // Not testing RX
  .uart_tx(uart_tx),
  .led(led),
  .enable_tx(enable_tx),
  .tx_data(test_data[idx]),
  .tx_done(tx_done)
);

reg [23:0] delay = 0;
reg [1:0] state = 0;

always @(posedge clk) begin
  case (state)
    0: begin
      enable_tx <= 1;       // start TX
      state <= 1;
    end
    1: begin
      enable_tx <= 0;       // clear after 1 cycle
      if (tx_done) begin
        if (idx == 15) begin
          state <= 2;       // last byte sent, go to delay state
        end else begin
          idx <= idx + 1;   // send next byte
          state <= 0;
        end
      end
    end
    2: begin                 // only after the entire test_data[] is done
      if (delay == 24'd5_000_000) begin
        delay <= 0;
        idx <= 0;           // start over
        state <= 0;
      end else
        delay <= delay + 1;
    end
  endcase
end
endmodule
