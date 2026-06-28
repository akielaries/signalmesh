// acm_top - ACM FPGA datapath (shared across boards).
//
// this is a COMPOSITION ROOT: it only instantiates blocks and wires them
// together - no register values, no DSP logic of its own. every block below is
// independent and talks over a documented interface:
//
//   fmc_wb_bridge : FMC bus            <-> Wishbone-16 master   (knows nothing else)
//   core / audio  : Wishbone-16 slaves (cheby-generated reg files)
//   dds_synth     : voice-control bus + tick -> mixed sample    (knows no registers)
//
// the board's top.v instantiates this, wires the physical FMC pins, drives the
// identity inputs (magic/fpga_id/version) for its target, and sets TICK_DIV for
// its clock.  to change the register layout, edit the ADDRESS DECODE section.

module acm_top #(
  parameter integer TICK_DIV = 562          // clk cycles per audio sample (27 MHz/562 ~= 48 kHz)
) (
  input  wire        clk,
  input  wire        rst,                    // active high

  // physical FMC bus (from the board top.v)
  inout  wire [15:0] FMC_AD,
  input  wire        FMC_NADV,
  input  wire        FMC_NOE,
  input  wire        FMC_NWE,
  input  wire        FMC_NE1,
  output wire        FMC_NWAIT,

  // identity register values - the board declares these (NOT baked in here)
  input  wire [15:0] magic_i,
  input  wire [15:0] fpga_id_i,
  input  wire [15:0] version_i,

  output wire [15:0] scratch_o               // core scratch reg (e.g. board LEDs)
);

  // ==========================================================================
  // 1. FMC -> Wishbone-16 master
  // ==========================================================================
  wire        wb_cyc, wb_stb, wb_we, wb_ack;
  wire [7:0]  wb_adr;
  wire [1:0]  wb_sel;
  wire [15:0] wb_wdata, wb_rdata;

  fmc_wb_bridge bridge_inst (
    .clk(clk), .rst(rst),
    .AD(FMC_AD), .NADV(FMC_NADV), .NOE(FMC_NOE), .NWE(FMC_NWE),
    .NE1(FMC_NE1), .NWAIT(FMC_NWAIT),
    .wb_cyc_o(wb_cyc), .wb_stb_o(wb_stb), .wb_adr_o(wb_adr), .wb_sel_o(wb_sel),
    .wb_we_o(wb_we), .wb_dat_o(wb_wdata), .wb_dat_i(wb_rdata), .wb_ack_i(wb_ack)
  );

  // ==========================================================================
  // 2. ADDRESS DECODE  <-- the register map. edit here to add/move blocks.
  //      core   : FMC byte 0x00..0x7F   (word address bit 6 = 0)
  //      audio  : FMC byte 0x80..0xFF   (word address bit 6 = 1)
  // ==========================================================================
  wire sel_audio = wb_adr[6];

  wire        core_cyc  = wb_cyc & ~sel_audio;
  wire        audio_cyc = wb_cyc &  sel_audio;
  wire [15:0] core_rdata, audio_rdata;
  wire        core_ack,   audio_ack;

  assign wb_rdata = sel_audio ? audio_rdata : core_rdata;
  assign wb_ack   = sel_audio ? audio_ack   : core_ack;

  // ==========================================================================
  // 3. core registers (identity / scratch) - values come from the board inputs
  // ==========================================================================
  core core_inst (
    .rst_n_i(~rst), .clk_i(clk),
    .wb_cyc_i(core_cyc), .wb_stb_i(core_cyc & wb_stb), .wb_adr_i(wb_adr[1:0]),
    .wb_sel_i(wb_sel), .wb_we_i(wb_we), .wb_dat_i(wb_wdata),
    .wb_ack_o(core_ack), .wb_err_o(), .wb_rty_o(), .wb_stall_o(), .wb_dat_o(core_rdata),
    .magic_i(magic_i), .fpga_id_i(fpga_id_i), .scratch_o(scratch_o), .version_i(version_i)
  );

  // ==========================================================================
  // 4. audio / voice registers - voice controls drive the DDS, sample reads back
  // ==========================================================================
  wire [8*32-1:0] v_freq;
  wire [8-1:0]    v_gate;
  wire [8*3-1:0]  v_wave;
  wire [8*8-1:0]  v_level;
  wire signed [15:0] dds_sample;

  // DAC-ready code: signed mix -> 12-bit unsigned (>>3 headroom + clamp), so the
  // STM32 can DMA it straight to the DAC with no CPU in the audio path.
  wire signed [16:0] dac_biased = ($signed(dds_sample) >>> 3) + 17'sd2048;
  wire [11:0] dac_code = dac_biased[16]       ? 12'd0    :   // negative -> 0
                         (|dac_biased[15:12]) ? 12'd4095 :   // > 4095 -> clamp
                         dac_biased[11:0];

  audio audio_inst (
    .rst_n_i(~rst), .clk_i(clk),
    .wb_cyc_i(audio_cyc), .wb_stb_i(audio_cyc & wb_stb), .wb_adr_i(wb_adr[5:0]),
    .wb_sel_i(wb_sel), .wb_we_i(wb_we), .wb_dat_i(wb_wdata),
    .wb_ack_o(audio_ack), .wb_err_o(), .wb_rty_o(), .wb_stall_o(), .wb_dat_o(audio_rdata),
    .ctrl_enable_o(), .ctrl_srate_o(),
    .status_active_voices_i(8'd0),
    .sample_i(dds_sample),
    .dac_i({4'b0, dac_code}),
    .voice_0_freq_o(v_freq[0*32 +: 32]), .voice_0_ctrl_gate_o(v_gate[0]),
    .voice_0_ctrl_wave_o(v_wave[0*3 +: 3]), .voice_0_ctrl_level_o(v_level[0*8 +: 8]),
    .voice_1_freq_o(v_freq[1*32 +: 32]), .voice_1_ctrl_gate_o(v_gate[1]),
    .voice_1_ctrl_wave_o(v_wave[1*3 +: 3]), .voice_1_ctrl_level_o(v_level[1*8 +: 8]),
    .voice_2_freq_o(v_freq[2*32 +: 32]), .voice_2_ctrl_gate_o(v_gate[2]),
    .voice_2_ctrl_wave_o(v_wave[2*3 +: 3]), .voice_2_ctrl_level_o(v_level[2*8 +: 8]),
    .voice_3_freq_o(v_freq[3*32 +: 32]), .voice_3_ctrl_gate_o(v_gate[3]),
    .voice_3_ctrl_wave_o(v_wave[3*3 +: 3]), .voice_3_ctrl_level_o(v_level[3*8 +: 8]),
    .voice_4_freq_o(v_freq[4*32 +: 32]), .voice_4_ctrl_gate_o(v_gate[4]),
    .voice_4_ctrl_wave_o(v_wave[4*3 +: 3]), .voice_4_ctrl_level_o(v_level[4*8 +: 8]),
    .voice_5_freq_o(v_freq[5*32 +: 32]), .voice_5_ctrl_gate_o(v_gate[5]),
    .voice_5_ctrl_wave_o(v_wave[5*3 +: 3]), .voice_5_ctrl_level_o(v_level[5*8 +: 8]),
    .voice_6_freq_o(v_freq[6*32 +: 32]), .voice_6_ctrl_gate_o(v_gate[6]),
    .voice_6_ctrl_wave_o(v_wave[6*3 +: 3]), .voice_6_ctrl_level_o(v_level[6*8 +: 8]),
    .voice_7_freq_o(v_freq[7*32 +: 32]), .voice_7_ctrl_gate_o(v_gate[7]),
    .voice_7_ctrl_wave_o(v_wave[7*3 +: 3]), .voice_7_ctrl_level_o(v_level[7*8 +: 8])
  );

  // ==========================================================================
  // 5. Fs sample tick (clock divider)
  // ==========================================================================
  reg [15:0] tickcnt;
  reg        tick;
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      tickcnt <= 16'd0;
      tick    <= 1'b0;
    end
    else if (tickcnt == TICK_DIV-1) begin
      tickcnt <= 16'd0;
      tick    <= 1'b1;
    end
    else begin
      tickcnt <= tickcnt + 16'd1;
      tick    <= 1'b0;
    end
  end

  // ==========================================================================
  // 6. DDS oscillator bank (voice bus in, mixed sample out)
  // ==========================================================================
  dds_synth dds_inst (
    .clk(clk), .rst(rst), .tick(tick),
    .freq_flat(v_freq), .gate_flat(v_gate), .wave_flat(v_wave), .level_flat(v_level),
    .sample_o(dds_sample), .sample_valid()
  );

endmodule
