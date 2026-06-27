// multiplexed-mode FMC slave (async, 16-bit) for the STM32H7 FMC.
//
// the STM32 drives the LOW address on AD[15:0] during NADV, then the same wires
// carry data. this slave latches the address, serves combinational reads, and
// latches writes. 16-bit bus.
//
// register map (16-bit, word-addressed; STM32 byte offset = word index * 2):
//   word 0     (0x60000000): ID        RO  0xACE1
//   word 1     (0x60000002): SCRATCH   RW
//   word 2     (0x60000004): LED_CTRL  RW  low 5 bits -> LEDs
//   word 3     (0x60000006): WAVE_SEL  RW  [1:0] 0=sine 1=saw 2=square 3=triangle
//   0x100-1FF  (0x60000200): AUDIO     RO  sample(index, WAVE_SEL), 12-bit in low bits
//
// the AUDIO region is one period of the selected waveform (256 x 12-bit). the
// STM32 reads it into a DAC buffer; WAVE_SEL picks the timbre. this is the
// minimal "wavetable generator" slice - DDS pitch + mixing come later.
//
// NADV/NOE/NWE/NE1 are asynchronous to clk; brought across with 2-FF synchronizers.

module fmc_mux_slave (
  input  wire        clk,        // 27 MHz fpga clock
  input  wire        rst,        // active high

  inout  wire [15:0] AD,         // 16-bit multiplexed address/data
  input  wire        NADV,       // NL  (active low) - address latch
  input  wire        NOE,        //     (active low) - read strobe
  input  wire        NWE,        //     (active low) - write strobe
  input  wire        NE1,        //     (active low) - chip select
  output wire        NWAIT,      //     (active low) - wait (inactive for PoC)

  output reg  [15:0] led_ctrl
);

  localparam [15:0] ID_VALUE = 16'hACE1;

  // ---- synchronize the async control strobes ----
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

  // ---- synchronize the AD inputs ----
  reg [15:0] ad_s0, ad_in;
  always @(posedge clk) begin
    ad_s0 <= AD;
    ad_in <= ad_s0;
  end

  // ---- latch the address while NADV is asserted ----
  reg [15:0] addr;
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      addr <= 16'd0;
    end
    else if (!nadv) begin
      addr <= ad_in;
    end
  end

  // ---- registers + write commit on the NWE rising edge ----
  reg [15:0] scratch;
  reg [1:0]  wave_sel;
  reg [15:0] wdata;
  reg        nwe_d;
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      scratch  <= 16'd0;
      led_ctrl <= 16'd0;
      wave_sel <= 2'd0;
      wdata    <= 16'd0;
      nwe_d    <= 1'b1;
    end
    else begin
      nwe_d <= nwe;
      if (!ne1 && !nwe) begin
        wdata <= ad_in;
      end
      if (!ne1 && nwe && !nwe_d) begin
        case (addr)
          16'd1: scratch  <= wdata;
          16'd2: led_ctrl <= wdata;
          16'd3: wave_sel <= wdata[1:0];
          default: ;
        endcase
      end
    end
  end

  // ---- waveform generator: one period of the selected wave, by index ----
  wire [11:0] sine_s;
  sine_lut u_sine (.addr(addr[7:0]), .data(sine_s));

  wire [6:0] tri_idx = addr[7] ? (7'd127 - addr[6:0]) : addr[6:0];
  reg  [11:0] wave_s;
  always @(*) begin
    case (wave_sel)
      2'd0:    wave_s = sine_s;                  // sine (LUT)
      2'd1:    wave_s = {addr[7:0], 4'b0};       // sawtooth  (0..4080)
      2'd2:    wave_s = addr[7] ? 12'd0 : 12'd4095;  // square
      2'd3:    wave_s = {tri_idx, 5'b0};         // triangle  (0..4064)
      default: wave_s = sine_s;
    endcase
  end

  // ---- combinational read mux ----
  reg [15:0] rdata;
  always @(*) begin
    if (addr[15:8] == 8'h01) begin
      rdata = {4'b0, wave_s};              // AUDIO region 0x100-0x1FF
    end
    else begin
      case (addr)
        16'd0:   rdata = ID_VALUE;
        16'd1:   rdata = scratch;
        16'd2:   rdata = led_ctrl;
        16'd3:   rdata = {14'b0, wave_sel};
        default: rdata = 16'h0000;
      endcase
    end
  end

  // ---- drive AD only during the read data phase ----
  wire drive_ad = (!ne1) && (!noe);
  assign AD = drive_ad ? rdata : 16'bz;

  assign NWAIT = 1'b1;

endmodule
