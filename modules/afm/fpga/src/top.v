// =============================================================================
// top.v — Gowin EMPU Cortex-M1 + GWCT debug module
//
// GWCT (Gowin Watch & Control Tool) is a UART-based debug master that can
// read and write APB-mapped registers independently of the Cortex-M1.
//
// APB bus arbitration (simple priority mux):
//   - GWCT has priority when it has an active transaction (gwct_apb_sel=1)
//   - Cortex-M1 APB bridge gets the bus otherwise
//
// This means: if GWCT is active, the Cortex-M1 cannot access APB until
// GWCT finishes its single transaction (~4-5 APB cycles). This is fine
// for debug purposes.
// =============================================================================


module top (
    input HCLK,
    input hwRstn,
    inout [15:0] GPIO,
    inout JTAG_7_SWDIO,
    inout JTAG_9_SWDCLK,
    input UART1RXD,
    output UART1TXD,
    output GWCT_TX,
    input  GWCT_RX,  
    output LOCKUP,
    output HALTED,
    inout BOOT_LED_A
);
    // =========================================================================
    // 0.5 second counter at 50mhz drives BOOT_LED
    // =========================================================================
    reg [24:0] counter;
    reg gpio1_out;

    always @(posedge HCLK or negedge hwRstn) begin
        if (!hwRstn) begin
            counter   <= 25'd0;
            gpio1_out <= 1'b0;
        end else begin
            if (counter == 25_000_000 - 1) begin
                counter   <= 25'd0;
                gpio1_out <= ~gpio1_out;
            end else begin
                counter <= counter + 1'b1;
            end
        end
    end

    assign BOOT_LED_A = gpio1_out;

    // =========================================================================
    // APB1 wires from Cortex-M1 APB bridge
    // =========================================================================
    wire [31:0] APB1PADDR;
    wire        APB1PENABLE;
    wire        APB1PWRITE;
    wire [3:0]  APB1PSTRB;
    wire [2:0]  APB1PPROT;
    wire [31:0] APB1PWDATA;
    wire [31:0] APB1PRDATA;
    wire        APB1PREADY;
    wire        APB1PSLVERR;
    wire        APB1PCLK;
    wire        APB1PRESET;
    wire        APB1PSEL;

    // =========================================================================
    // AHB1 wires from Cortex-M1 APB bridge
    // =========================================================================
/*
    wire [31:0] AHB1HRDATA; //input [31:0] AHB1HRDATA
    wire AHB1HREADYOUT; //input AHB1HREADYOUT
	wire [1:0] AHB1HRESP; //input [1:0] AHB1HRESP
	wire [1:0] AHB1HTRANS; //output [1:0] AHB1HTRANS
	wire [2:0] AHB1HBURST; //output [2:0] AHB1HBURST
	wire [3:0] AHB1HPROT; //output [3:0] AHB1HPROT
	wire [2:0] AHB1HSIZE; //output [2:0] AHB1HSIZE
	wire AHB1HWRITE; //output AHB1HWRITE
	wire AHB1HREADYMUX; //output AHB1HREADYMUX
	wire [3:0] AHB1HMASTER; //output [3:0] AHB1HMASTER
	wire AHB1HMASTLOCK; //output AHB1HMASTLOCK
	wire [31:0] AHB1HADDR; //output [31:0] AHB1HADDR
    wire [31:0] AHB1HWDATA; //output [31:0] AHB1HWDATA
    wire AHB1HSEL; //output AHB1HSEL
	wire AHB1HCLK; //output AHB1HCLK
    wire AHB1HRESET; //output AHB1HRESET
*/
    // ------------------------------------------------------------
    // Cortex-M1 instantiation
    // ------------------------------------------------------------
    Gowin_EMPU_M1_Top Cortex_M1_instance(
        .LOCKUP(LOCKUP),
        .HALTED(HALTED),
        .GPIO(GPIO),
        .JTAG_7(JTAG_7_SWDIO),
        .JTAG_9(JTAG_9_SWDCLK),
        .UART1RXD(UART1RXD),
        .UART1TXD(UART1TXD),
 
/*           
        // AHB1 interface
		.AHB1HRDATA(AHB1HRDATA), //input [31:0] AHB1HRDATA
		.AHB1HREADYOUT(AHB1HREADYOUT), //input AHB1HREADYOUT
		.AHB1HRESP(AHB1HRESP), //input [1:0] AHB1HRESP
		.AHB1HTRANS(AHB1HTRANS), //output [1:0] AHB1HTRANS
		.AHB1HBURST(AHB1HBURST), //output [2:0] AHB1HBURST
		.AHB1HPROT(AHB1HPROT), //output [3:0] AHB1HPROT
		.AHB1HSIZE(AHB1HSIZE), //output [2:0] AHB1HSIZE
		.AHB1HWRITE(AHB1HWRITE), //output AHB1HWRITE
		.AHB1HREADYMUX(AHB1HREADYMUX), //output AHB1HREADYMUX
		.AHB1HMASTER(AHB1HMASTER), //output [3:0] AHB1HMASTER
		.AHB1HMASTLOCK(AHB1HMASTLOCK), //output AHB1HMASTLOCK
		.AHB1HADDR(AHB1HADDR), //output [31:0] AHB1HADDR
		.AHB1HWDATA(AHB1HWDATA), //output [31:0] AHB1HWDATA
		.AHB1HSEL(AHB1HSEL), //output AHB1HSEL
		.AHB1HCLK(AHB1HCLK), //output AHB1HCLK
		.AHB1HRESET(AHB1HRESET), //output AHB1HRESET
*/
        // APB1 interface
        .APB1PADDR(APB1PADDR),
        .APB1PENABLE(APB1PENABLE),
        .APB1PWRITE(APB1PWRITE),
        .APB1PSTRB(APB1PSTRB),
//        .APB1PPROT(APB1PPROT),
        .APB1PWDATA(APB1PWDATA),
        .APB1PRDATA(APB1PRDATA),
        .APB1PREADY(APB1PREADY),
        .APB1PSLVERR(APB1PSLVERR),
        .APB1PCLK(APB1PCLK),
        .APB1PRESET(APB1PRESET),
        .APB1PSEL(APB1PSEL),

        .HCLK(HCLK),
        .hwRstn(hwRstn)
    );
    // =========================================================================
    // AHB1 RAM (16 KB scratch at 0x8000_0000)
    // =========================================================================
    wire [31:0] ahb_ram_hrdata;
    wire        ahb_ram_hreadyout;
    wire [1:0]  ahb_ram_hresp;

    ahb_sram #(
        .SIZE      (16384),
        .BASE_ADDR (32'h8000_0000)
    ) ahb_ram_inst (
        .HCLK       (HCLK),
        .HRESETn    (hwRstn),

        .HADDR      (AHB1HADDR),
        .HWRITE     (AHB1HWRITE),
        .HSIZE      (AHB1HSIZE),
        .HTRANS     (AHB1HTRANS),
        .HWDATA     (AHB1HWDATA),
        .HSEL       (AHB1HSEL),        // use CPU's internal select if it covers 0x8000_0000

        .HRDATA     (ahb_ram_hrdata),
        .HREADYOUT  (ahb_ram_hreadyout),
        .HRESP      (ahb_ram_hresp)
    );

    // Connect the RAM outputs to the CPU inputs
    assign AHB1HRDATA    = ahb_ram_hrdata;
    assign AHB1HREADYOUT = ahb_ram_hreadyout;
    assign AHB1HRESP     = ahb_ram_hresp;

    // =========================================================================
    // GWCT debug bridge debug master with N-bus support
    // =========================================================================
    // Single bus configuration
    wire [31:0] gwct_PADDR;
    wire        gwct_PSEL;
    wire        gwct_PENABLE;
    wire        gwct_PWRITE;
    wire [31:0] gwct_PWDATA;
    wire [3:0]  gwct_PSTRB;
    wire [2:0]  gwct_PPROT;

    // Slave response wires
    wire [31:0] slave_PRDATA;
    wire        slave_PREADY;
    wire        slave_PSLVERR;

    gwct_debug_bridge_n #(
        .CLK_HZ(50_000_000),
        .BAUD(115_200),
        .NUM_APB_BUSES(1),          // Currently using 1 bus (APB1)
        .ADDR_BITS(28)              // Address decode uses addr[31:28]
    ) gwct_inst (
        .clk        (HCLK),
        .rstn       (hwRstn),
        
        // UART pins
        .uart_rx    (GWCT_RX),
        .uart_tx    (GWCT_TX),
        
        // APB master signals (single bus = simple wires)
        .PADDR      (gwct_PADDR),
        .PSEL       (gwct_PSEL),
        .PENABLE    (gwct_PENABLE),
        .PWRITE     (gwct_PWRITE),
        .PWDATA     (gwct_PWDATA),
        .PSTRB      (gwct_PSTRB),
        .PPROT      (gwct_PPROT),
        .PRDATA     (slave_PRDATA),
        .PREADY     (slave_PREADY),
        .PSLVERR    (slave_PSLVERR)
    );

    // =========================================================================
    // APB bus mux — GWCT takes priority over Cortex-M1
    //
    //   gwct_apb_sel = 1  →  GWCT drives the bus
    //   gwct_apb_sel = 0  →  Cortex-M1 drives the bus
    // =========================================================================
    wire gwct_apb_sel = gwct_PSEL;

    wire [31:0] mux_PADDR   = gwct_apb_sel ? gwct_PADDR   : APB1PADDR;
    wire        mux_PSEL    = gwct_apb_sel ? gwct_PSEL    : APB1PSEL;
    wire        mux_PENABLE = gwct_apb_sel ? gwct_PENABLE : APB1PENABLE;
    wire        mux_PWRITE  = gwct_apb_sel ? gwct_PWRITE  : APB1PWRITE;
    wire [31:0] mux_PWDATA  = gwct_apb_sel ? gwct_PWDATA  : APB1PWDATA;

    // Feed response back to Cortex-M1 (stall if GWCT owns bus)
    assign APB1PRDATA  = slave_PRDATA;
    assign APB1PREADY  = gwct_apb_sel ? 1'b0 : slave_PREADY;
    assign APB1PSLVERR = slave_PSLVERR;

    // =========================================================================
    // apb_memmap slave — sees muxed bus
    // =========================================================================
    apb_memmap apb_memmap_inst (
        .APBCLK   (APB1PCLK),
        .APBRESET (APB1PRESET),
        .PADDR    (mux_PADDR),
        .PSEL     (mux_PSEL),
        .PENABLE  (mux_PENABLE),
        .PWRITE   (mux_PWRITE),
        .PWDATA   (mux_PWDATA),
        .PRDATA   (slave_PRDATA),
        .PREADY   (slave_PREADY),
        .PSLVERR  (slave_PSLVERR)
    );

endmodule