// polyphonic DDS oscillator bank (time-multiplexed).
//
// on each sample tick it walks the NVOICES voices, one per clock: advances that
// voice's phase accumulator by its tuning word (freq), selects its waveform from
// the top phase bits, scales by level (zero if not gated), and sums into the
// mixed output sample. one sine LUT and one multiplier are shared across voices.
//
// the per-voice control comes flattened from the cheby audio reg file:
//   freq  : 32-bit DDS tuning word   (inc = note_freq * 2^32 / Fs)
//   gate  : 1 = sounding
//   wave  : 0=sine 1=saw 2=square 3=triangle
//   level : 8-bit amplitude
//
// samples are signed; the final mix is summed with NVOICES headroom (>>3 for 8).

module dds_synth #(
  parameter integer NVOICES  = 8,
  parameter integer PHASE_W  = 32,
  parameter integer SAMPLE_W = 16
) (
  input  wire                       clk,
  input  wire                       rst,        // active high
  input  wire                       tick,       // 1-cycle pulse at the sample rate

  input  wire [NVOICES*PHASE_W-1:0] freq_flat,
  input  wire [NVOICES-1:0]         gate_flat,
  input  wire [NVOICES*3-1:0]       wave_flat,
  input  wire [NVOICES*8-1:0]       level_flat,

  output reg  signed [SAMPLE_W-1:0] sample_o,
  output reg                        sample_valid
);

  reg  [PHASE_W-1:0] phase [0:NVOICES-1];
  integer i;

  localparam ST_IDLE = 1'b0, ST_RUN = 1'b1;
  reg        state;
  reg  [3:0] vcnt;
  reg  signed [SAMPLE_W+4:0] mix;        // headroom to sum NVOICES samples

  // ---- fields of the voice currently being processed ----
  wire [PHASE_W-1:0] cur_freq  = freq_flat [vcnt*PHASE_W +: PHASE_W];
  wire               cur_gate  = gate_flat [vcnt];
  wire [2:0]         cur_wave  = wave_flat [vcnt*3 +: 3];
  wire [7:0]         cur_level = level_flat[vcnt*8 +: 8];

  wire [PHASE_W-1:0] phase_next = phase[vcnt] + cur_freq;
  wire [7:0]         phase_top  = phase_next[PHASE_W-1 -: 8];

  // shared sine LUT (12-bit unsigned, centered at 0x800)
  wire [11:0] sine_u;
  sine_lut u_sine (.addr(phase_top), .data(sine_u));

  // ---- waveform select -> signed SAMPLE_W (XOR-the-MSB centers offset-binary) ----
  wire [15:0] saw_u    = phase_next[PHASE_W-1 -: 16];
  wire [15:0] tri_ramp = phase_next[PHASE_W-1] ? ~phase_next[PHASE_W-2 -: 16]
                                               :  phase_next[PHASE_W-2 -: 16];
  reg signed [SAMPLE_W-1:0] wave_s;
  always @(*) begin
    case (cur_wave[1:0])
      2'd0: wave_s = $signed({(sine_u ^ 12'h800), 4'b0});            // sine
      2'd1: wave_s = $signed(saw_u ^ 16'h8000);                      // sawtooth
      2'd2: wave_s = phase_next[PHASE_W-1] ? 16'sh7FFF : -16'sh8000; // square
      2'd3: wave_s = $signed(tri_ramp ^ 16'h8000);                   // triangle
      default: wave_s = {SAMPLE_W{1'b0}};
    endcase
  end

  // scale by level (and gate): scaled = (prod >>> 8), taken as an explicit slice
  // so the safe narrowing doesn't raise a truncation warning (|prod>>>8| < 2^15)
  wire signed [SAMPLE_W+8:0] prod   = wave_s * $signed({1'b0, cur_level});
  wire signed [SAMPLE_W-1:0] scaled = cur_gate ? prod[SAMPLE_W+7:8] : {SAMPLE_W{1'b0}};

  // final mix: sum of the NVOICES voices, /8 (>>3) for headroom; explicit slice
  // = (mix_sum >>> 3) low SAMPLE_W bits (|mix_sum| < 2^18, so it fits)
  wire signed [SAMPLE_W+5:0] mix_sum = mix + scaled;
  wire signed [SAMPLE_W-1:0] mix_out = mix_sum[SAMPLE_W+2:3];

  always @(posedge clk or posedge rst) begin
    if (rst) begin
      state        <= ST_IDLE;
      vcnt         <= 4'd0;
      mix          <= 0;
      sample_o     <= 0;
      sample_valid <= 1'b0;
      for (i = 0; i < NVOICES; i = i + 1) phase[i] <= {PHASE_W{1'b0}};
    end
    else begin
      sample_valid <= 1'b0;
      case (state)
        ST_IDLE: begin
          if (tick) begin
            vcnt  <= 4'd0;
            mix   <= 0;
            state <= ST_RUN;
          end
        end
        ST_RUN: begin
          phase[vcnt] <= phase_next;                 // advance this voice
          if (vcnt == NVOICES-1) begin
            sample_o     <= mix_out;                  // include last voice, sum/8
            sample_valid <= 1'b1;
            state        <= ST_IDLE;
          end
          else begin
            mix  <= mix + scaled;
            vcnt <= vcnt + 4'd1;
          end
        end
      endcase
    end
  end

endmodule
