// cocotb wrapper for acm_top: small TICK_DIV so audio samples come fast in sim,
// and the identity inputs exposed as ports for the python test to drive (a real
// board top.v ties them to constants).

module acm_tb (
  input  wire        clk,
  input  wire        rst,

  inout  wire [15:0] FMC_AD,
  input  wire        FMC_NADV,
  input  wire        FMC_NOE,
  input  wire        FMC_NWE,
  input  wire        FMC_NE1,
  output wire        FMC_NWAIT,

  input  wire [15:0] magic_i,
  input  wire [15:0] fpga_id_i,
  input  wire [15:0] version_i,
  output wire [15:0] scratch_o
);

  acm_top #(.TICK_DIV(8)) dut (
    .clk(clk), .rst(rst),
    .FMC_AD(FMC_AD), .FMC_NADV(FMC_NADV), .FMC_NOE(FMC_NOE), .FMC_NWE(FMC_NWE),
    .FMC_NE1(FMC_NE1), .FMC_NWAIT(FMC_NWAIT),
    .magic_i(magic_i), .fpga_id_i(fpga_id_i), .version_i(version_i),
    .scratch_o(scratch_o)
  );

endmodule
