// multiplexed-mode FMC slave (async, 16-bit) for the STM32H7 FMC.
//
// the STM32 drives the LOW address on AD[15:0] during NADV, then the same wires
// carry data. this slave latches the address, serves combinational reads, and
// latches writes. 16-bit bus.
//
// register map (16-bit, word-addressed; STM32 byte offset = word index * 2):
//   word 0 (0x60000000): ID       RO  0xACE1
//   word 1 (0x60000002): SCRATCH  RW
//   word 2 (0x60000004): LED_CTRL RW  low 5 bits -> LEDs
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
  reg [15:0] wdata;
  reg        nwe_d;
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      scratch  <= 16'd0;
      led_ctrl <= 16'd0;
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
          default: ;
        endcase
      end
    end
  end

  // ---- combinational read mux ----
  reg [15:0] rdata;
  always @(*) begin
    case (addr)
      16'd0:   rdata = ID_VALUE;
      16'd1:   rdata = scratch;
      16'd2:   rdata = led_ctrl;
      default: rdata = 16'h0000;
    endcase
  end

  // ---- drive AD only during the read data phase ----
  wire drive_ad = (!ne1) && (!noe);
  assign AD = drive_ad ? rdata : 16'bz;

  assign NWAIT = 1'b1;

endmodule
