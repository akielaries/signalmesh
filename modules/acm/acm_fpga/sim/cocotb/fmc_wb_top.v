// cocotb TB top: integrates the FMC->wb-16 bridge with the cheby `core` reg file.
// the RO register values are exposed as ports so the python test can drive them
// (a real board top.v hardwires them: magic=0xACE1, fpga_id per target).

module fmc_wb_top (
  input  wire        clk,
  input  wire        rst,

  inout  wire [15:0] AD,
  input  wire        NADV,
  input  wire        NOE,
  input  wire        NWE,
  input  wire        NE1,
  output wire        NWAIT,

  input  wire [15:0] magic_i,
  input  wire [15:0] fpga_id_i,
  input  wire [15:0] version_i
);

  wire        wb_cyc, wb_stb, wb_we, wb_ack;
  wire [7:0]  wb_adr;
  wire [1:0]  wb_sel;
  wire [15:0] wb_m2s, wb_s2m;

  fmc_wb_bridge bridge (
    .clk(clk), .rst(rst),
    .AD(AD), .NADV(NADV), .NOE(NOE), .NWE(NWE), .NE1(NE1), .NWAIT(NWAIT),
    .wb_cyc_o(wb_cyc), .wb_stb_o(wb_stb), .wb_adr_o(wb_adr), .wb_sel_o(wb_sel),
    .wb_we_o(wb_we), .wb_dat_o(wb_m2s), .wb_dat_i(wb_s2m), .wb_ack_i(wb_ack)
  );

  core u_core (
    .rst_n_i(~rst), .clk_i(clk),
    .wb_cyc_i(wb_cyc), .wb_stb_i(wb_stb), .wb_adr_i(wb_adr[1:0]), .wb_sel_i(wb_sel),
    .wb_we_i(wb_we), .wb_dat_i(wb_m2s),
    .wb_ack_o(wb_ack), .wb_err_o(), .wb_rty_o(), .wb_stall_o(), .wb_dat_o(wb_s2m),
    .magic_i(magic_i), .fpga_id_i(fpga_id_i), .scratch_o(), .version_i(version_i)
  );

endmodule
