// multiplexed-mode FMC slave (async) for the STM32H7 FMC.
//
// the STM32 drives the LOW address on AD[15:0] during the NADV (address) phase,
// then the same wires carry data. this slave latches the address, serves
// combinational reads, and latches writes. 16-bit bus.
//
// register map (16-bit, word-addressed). STM32 byte address = word index * 2:
//   word 0  (0x60000000): ID       RO  0xACE1   (presence handshake)
//   word 1  (0x60000002): SCRATCH  RW          (write/read-back proof)
//   word 2  (0x60000004): LED_CTRL RW          (low bits drive the LEDs)
//
// NADV/NOE/NWE/NE1 are asynchronous to clk; each is brought across with a 2-FF
// synchronizer. run the FMC slow (generous ADDSET/DATAST) for the breadboard
// proof of concept. NWAIT is held inactive for now (fixed FMC timing).

module fmc_mux_slave (
  input  wire        clk,        // fpga clock (27 MHz is fine for slow FMC)
  input  wire        rst,        // active high

  // FMC multiplexed async bus (from the STM32)
  inout  wire [15:0] AD,         // address/data multiplexed
  input  wire        NADV,       // address valid / latch  (active low)
  input  wire        NOE,        // output enable / read   (active low)
  input  wire        NWE,        // write enable / write   (active low)
  input  wire        NE1,        // chip select            (active low)
  output wire        NWAIT,      // wait                   (active low)

  // application-visible register outputs
  output reg  [15:0] led_ctrl
);

  localparam [15:0] ID_VALUE = 16'hACE1;

  // ---- synchronize the async control strobes (2 FF each) -------------------
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

  // ---- synchronize the AD inputs (address latch + write data) --------------
  reg [15:0] ad_s0, ad_in;
  always @(posedge clk) begin
    ad_s0 <= AD;
    ad_in <= ad_s0;
  end

  // ---- latch the address while NADV is asserted ----------------------------
  reg [15:0] addr;
  always @(posedge clk or posedge rst) begin
    if (rst) begin
      addr <= 16'd0;
    end
    else if (!nadv) begin
      addr <= ad_in;               // address is valid on AD while NADV is low
    end
  end

  // ---- registers + write commit on the NWE rising edge ---------------------
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
        wdata <= ad_in;            // capture data while NWE is low
      end
      if (!ne1 && nwe && !nwe_d) begin   // NWE rising edge -> commit
        case (addr)
          16'd1: scratch  <= wdata;
          16'd2: led_ctrl <= wdata;
          default: ;               // ID is read-only; unknown addrs ignored
        endcase
      end
    end
  end

  // ---- combinational read mux ----------------------------------------------
  reg [15:0] rdata;
  always @(*) begin
    case (addr)
      16'd0:   rdata = ID_VALUE;
      16'd1:   rdata = scratch;
      16'd2:   rdata = led_ctrl;
      default: rdata = 16'h0000;
    endcase
  end

  // ---- drive AD only during the read data phase (NE1 low, NOE low) ---------
  wire drive_ad = (!ne1) && (!noe);
  assign AD = drive_ad ? rdata : 16'bz;

  // ---- NWAIT inactive (active-low, deasserted) for the slow PoC ------------
  assign NWAIT = 1'b1;

endmodule
