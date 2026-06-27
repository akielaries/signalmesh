// DIAGNOSTIC BUILD - bisecting the FMC read regression.
// this is the known-good "bridge + core direct" config (no acm_top, no decode,
// no audio/dds), but using the CURRENT widened bridge (wb_adr_o[7:0]).
//   - if test_fmc_core reads MAGIC=0xACE1 OK -> the bridge is fine, the bug is
//     in acm_top (the decode mux / audio presence).
//   - if it still reads wrong -> the bridge widening itself is the culprit.
//
// IMPORTANT for this diagnostic: in the .gprj, DISABLE (uncheck) acm_top.v,
// audio_regs.v, dds_synth.v, sine_lut.v so `top` is the only top candidate.
// restore them + the acm_top version of this file afterward.

module top (
  input  HCLK,
  input  HRST,

  output [5:0] LED,

  inout  [15:0] FMC_AD,
  input         FMC_NADV,
  input         FMC_NOE,
  input         FMC_NWE,
  input         FMC_NE1,
  output        FMC_NWAIT,

  output        UART_TX
);

  localparam integer CLK_FREQ = 27_000_000;

  boot_blinker #(.HALF_PERIOD(CLK_FREQ / 2)) boot_blink_inst (
    .HCLK(HCLK), .HRST(HRST), .BOOT_LED(LED[5])
  );

  stm32_uart_test #(.CLK_FREQ(CLK_FREQ)) uart_test_inst (
    .HCLK(HCLK), .HRST(HRST), .UART_TX(UART_TX)
  );

  // ---- FMC -> wb bridge -> core, DIRECT (no decode, no audio) ----
  wire        wb_cyc, wb_stb, wb_we, wb_ack;
  wire [7:0]  wb_adr;
  wire [1:0]  wb_sel;
  wire [15:0] wb_wdata, wb_rdata;
  wire [15:0] scratch;

  fmc_wb_bridge bridge_inst (
    .clk(HCLK), .rst(HRST),
    .AD(FMC_AD), .NADV(FMC_NADV), .NOE(FMC_NOE), .NWE(FMC_NWE),
    .NE1(FMC_NE1), .NWAIT(FMC_NWAIT),
    .wb_cyc_o(wb_cyc), .wb_stb_o(wb_stb), .wb_adr_o(wb_adr), .wb_sel_o(wb_sel),
    .wb_we_o(wb_we), .wb_dat_o(wb_wdata), .wb_dat_i(wb_rdata), .wb_ack_i(wb_ack)
  );

  core core_inst (
    .rst_n_i(~HRST), .clk_i(HCLK),
    .wb_cyc_i(wb_cyc), .wb_stb_i(wb_stb), .wb_adr_i(wb_adr[1:0]),
    .wb_sel_i(wb_sel), .wb_we_i(wb_we), .wb_dat_i(wb_wdata),
    .wb_ack_o(wb_ack), .wb_err_o(), .wb_rty_o(), .wb_stall_o(), .wb_dat_o(wb_rdata),
    .magic_i(16'hACE1), .fpga_id_i(16'h2018), .scratch_o(scratch), .version_i(16'h0001)
  );

  assign LED[4:0] = ~scratch[4:0];

endmodule
