/**
  * UART component block test bench
  */

`timescale 1ns/1ps


module test();
  reg clk = 0;
  reg uart_rx = 1;
  wire uart_tx;
  wire [5:0] led;
  reg enable_tx = 0;
  reg [7:0] tx_data = 0;
  wire tx_done;

  uart #(.BAUD_DIV(234)) u (
    .clk(clk),
    .uart_rx(uart_rx),
    .uart_tx(uart_tx),
    .led(led),
    .enable_tx(1'b0),     // or your testbench-controlled signal
    .tx_data(8'b0),       // or your test data here
    .tx_done()            // leave unconnected if not used
  );
  always
    #1  clk = ~clk;

  initial begin
    $display("Starting UART RX");
    $monitor("LED Value %b", led);
    #10 uart_rx=0;
    #16 uart_rx=1;
    #16 uart_rx=0;
    #16 uart_rx=0;
    #16 uart_rx=0;
    #16 uart_rx=0;
    #16 uart_rx=1;
    #16 uart_rx=1;
    #16 uart_rx=0;
    #16 uart_rx=1;
    #1000 $finish;
  end

  initial begin
    $dumpfile("uart.vcd");
    $dumpvars(0,test);
  end
endmodule
