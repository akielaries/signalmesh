// testbench for fmc_mux_slave: mimics the STM32 FMC multiplexed async cycles
// and checks ID read, scratch write/read-back, and the LED_CTRL register.
//
// run (icarus verilog):
//   iverilog -o fmc_tb ../src/fmc_mux_slave.v fmc_mux_slave_tb.v && vvp fmc_tb
// view waveforms: gtkwave fmc_tb.vcd

`timescale 1ns/1ps

module fmc_mux_slave_tb;

  reg         clk = 1'b0;
  reg         rst;
  reg         NE1, NOE, NWE, NADV;
  wire        NWAIT;
  wire [15:0] led_ctrl;

  // bidirectional AD: the testbench (master) drives address + write data;
  // the DUT drives read data
  reg  [15:0] tb_ad_out;
  reg         tb_ad_oe;
  wire [15:0] AD = tb_ad_oe ? tb_ad_out : 16'bz;

  fmc_mux_slave dut (
    .clk(clk), .rst(rst),
    .AD(AD), .NADV(NADV), .NOE(NOE), .NWE(NWE), .NE1(NE1), .NWAIT(NWAIT),
    .led_ctrl(led_ctrl)
  );

  always #10 clk = ~clk;          // 50 MHz sim clock

  integer errors = 0;

  task wait_clocks(input integer n);
    begin
      repeat (n) @(posedge clk);
    end
  endtask

  // one multiplexed write cycle
  task fmc_write(input [15:0] a, input [15:0] d);
    begin
      @(negedge clk);
      NE1 = 1'b0;
      // address phase: drive the address, pulse NADV low
      NADV = 1'b0; tb_ad_out = a; tb_ad_oe = 1'b1;
      wait_clocks(6);
      NADV = 1'b1;                 // address latched
      wait_clocks(4);
      // data phase: master drives write data, pulse NWE low
      tb_ad_out = d;
      NWE = 1'b0;
      wait_clocks(6);
      NWE = 1'b1;                  // rising edge -> slave commits
      wait_clocks(4);
      tb_ad_oe = 1'b0;            // release the bus
      NE1 = 1'b1;
      wait_clocks(6);
    end
  endtask

  // one multiplexed read cycle, with self-check
  task fmc_read(input [15:0] a, input [15:0] expect_d);
    reg [15:0] got;
    begin
      @(negedge clk);
      NE1 = 1'b0;
      // address phase
      NADV = 1'b0; tb_ad_out = a; tb_ad_oe = 1'b1;
      wait_clocks(6);
      NADV = 1'b1;
      wait_clocks(2);
      tb_ad_oe = 1'b0;           // master releases bus for turnaround
      wait_clocks(4);
      // data phase: assert NOE, let the slave drive, then sample
      NOE = 1'b0;
      wait_clocks(8);
      got = AD;
      NOE = 1'b1;
      NE1 = 1'b1;
      wait_clocks(6);
      if (got === expect_d) begin
        $display("PASS  read word %0d = 0x%04x", a, got);
      end
      else begin
        $display("FAIL  read word %0d = 0x%04x (expected 0x%04x)", a, got, expect_d);
        errors = errors + 1;
      end
    end
  endtask

  initial begin
    $dumpfile("fmc_tb.vcd");
    $dumpvars(0, fmc_mux_slave_tb);

    // idle bus
    NE1 = 1'b1; NOE = 1'b1; NWE = 1'b1; NADV = 1'b1;
    tb_ad_oe = 1'b0; tb_ad_out = 16'd0;

    rst = 1'b1; wait_clocks(5);
    rst = 1'b0; wait_clocks(5);

    fmc_read (16'd0, 16'hACE1);   // ID handshake
    fmc_read (16'd1, 16'h0000);   // scratch starts at 0
    fmc_write(16'd1, 16'h1234);   // write scratch
    fmc_read (16'd1, 16'h1234);   // read it back
    fmc_write(16'd2, 16'h002A);   // write LED_CTRL
    fmc_read (16'd2, 16'h002A);   // read it back

    if (led_ctrl === 16'h002A) begin
      $display("PASS  led_ctrl output = 0x%04x", led_ctrl);
    end
    else begin
      $display("FAIL  led_ctrl output = 0x%04x (expected 0x002A)", led_ctrl);
      errors = errors + 1;
    end

    if (errors == 0) begin
      $display("--- ALL TESTS PASSED ---");
    end
    else begin
      $display("--- %0d FAILURE(S) ---", errors);
    end
    $finish;
  end

  // watchdog
  initial begin
    #200000;
    $display("FAIL  timeout");
    $finish;
  end

endmodule
