// FMC multiplexed-mode (async, 16-bit) front-end -> Wishbone-16 master.
// bridges the STM32 FMC muxed bus to a cheby-generated wb-16 register file.
//
// the STM32 drives the low (word) address on AD[15:0] during NADV, then the same
// wires carry data. the async strobes are synced into clk; each FMC access turns
// into ONE wb cycle to the reg file, which acks within a couple clocks - the
// FMC's slow async timing easily covers that latency.
// see: https://cdn.opencores.org/downloads/wbspec_b3.pdf#page=110&zoom=100,0,0
// A.6.2 Simple 16-bit SLAVE Output Port With 16-bit Granularity 



module fmc_wb_bridge (
  input  wire        clk,          // 27 MHz fpga clock
  input  wire        rst,          // active high

  // ---- FMC (muxed, async) ----
  inout  wire [15:0] AD,
  input  wire        NADV,         // address latch  (active low)
  input  wire        NOE,          // read strobe    (active low)
  input  wire        NWE,          // write strobe   (active low)
  input  wire        NE1,          // chip select    (active low)
  output wire        NWAIT,        // wait (held inactive for PoC)

  // ---- Wishbone-16 master (to the cheby `core` reg file) ----
  output reg         wb_cyc_o,
  output reg         wb_stb_o,
  output reg  [7:0]  wb_adr_o,     // word address (covers the composed reg space)
  output wire [1:0]  wb_sel_o,
  output reg         wb_we_o,
  output reg  [15:0] wb_dat_o,
  input  wire [15:0] wb_dat_i,
  input  wire        wb_ack_i
);

  // ---- 2-FF synchronize the async control strobes ----
  reg [1:0] ne1_s, noe_s, nwe_s, nadv_s;
  always @(posedge clk) begin
    ne1_s  <= {ne1_s[0],  NE1};
    noe_s  <= {noe_s[0],  NOE};
    nwe_s  <= {nwe_s[0],  NWE};
    nadv_s <= {nadv_s[0], NADV};
  end
  wire ne1  = ne1_s[1];
  wire noe  = noe_s[1];
  wire nwe  = nwe_s[1];
  wire nadv = nadv_s[1];

  // ---- synchronize AD ----
  reg [15:0] ad_s0, ad_in;
  always @(posedge clk) begin
    ad_s0 <= AD;
    ad_in <= ad_s0;
  end

  // ---- latch the (word) address while NADV is asserted ----
  reg [15:0] addr;
  always @(posedge clk or posedge rst) begin
    if (rst)        addr <= 16'd0;
    else if (!nadv) addr <= ad_in;
  end

  // ---- capture write data while NWE is asserted ----
  reg [15:0] wdata;
  always @(posedge clk) begin
    if (!ne1 && !nwe) wdata <= ad_in;
  end

  // ---- FSM: one wb cycle per FMC access ----
  localparam ST_IDLE = 2'd0, ST_RD = 2'd1, ST_WR = 2'd2;
  reg [1:0]  state;
  reg        nwe_d;
  reg        rd_done;
  reg [15:0] rd_data;

  assign wb_sel_o = 2'b11;

  always @(posedge clk or posedge rst) begin
    if (rst) begin
      state    <= ST_IDLE;
      wb_cyc_o <= 1'b0;
      wb_stb_o <= 1'b0;
      wb_we_o  <= 1'b0;
      wb_adr_o <= 8'd0;
      wb_dat_o <= 16'd0;
      rd_done  <= 1'b0;
      rd_data  <= 16'd0;
      nwe_d    <= 1'b1;
    end
    else begin
      nwe_d <= nwe;

      case (state)
        ST_IDLE: begin
          wb_cyc_o <= 1'b0;
          wb_stb_o <= 1'b0;
          if (!ne1 && !noe && !rd_done) begin
            wb_cyc_o <= 1'b1;            // start read cycle
            wb_stb_o <= 1'b1;
            wb_we_o  <= 1'b0;
            wb_adr_o <= addr[7:0];
            state    <= ST_RD;
          end
          else if (!ne1 && nwe && !nwe_d) begin
            wb_cyc_o <= 1'b1;            // NWE rising edge -> commit write
            wb_stb_o <= 1'b1;
            wb_we_o  <= 1'b1;
            wb_adr_o <= addr[7:0];
            wb_dat_o <= wdata;
            state    <= ST_WR;
          end
        end

        ST_RD: begin
          if (wb_ack_i) begin
            rd_data  <= wb_dat_i;
            rd_done  <= 1'b1;
            wb_cyc_o <= 1'b0;
            wb_stb_o <= 1'b0;
            state    <= ST_IDLE;
          end
        end

        ST_WR: begin
          if (wb_ack_i) begin
            wb_cyc_o <= 1'b0;
            wb_stb_o <= 1'b0;
            state    <= ST_IDLE;
          end
        end

        default: state <= ST_IDLE;
      endcase

      // re-arm the read once the read strobe deasserts
      if (noe) rd_done <= 1'b0;
    end
  end

  // ---- drive AD during the read data phase ----
  wire drive_ad = (!ne1) && (!noe);
  assign AD = drive_ad ? rd_data : 16'bz;

  assign NWAIT = 1'b1;

endmodule
