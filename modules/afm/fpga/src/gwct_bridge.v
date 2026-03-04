// =============================================================================
// gwct_debug_bridge_n.v — Scalable GWCT debug bridge (1 to N APB buses)
//
// Verilog-2001 compatible version using flattened arrays instead of packed.
// Set NUM_APB_BUSES to whatever you need (1, 2, 7, 16, 32, etc.)
//
// Address decode uses a simple scheme:
//   - Each bus gets an equal-sized address window
//   - Bus N is selected when: (addr >> ADDR_BITS) == N
//
// Example with configurable address bits:
//   ADDR_BITS = 28 means upper 4 bits select the bus (16 buses max)
//   ADDR_BITS = 24 means upper 8 bits select the bus (256 buses max)
//
// Example instantiation with 1 bus:
//
//   wire [31:0] gwct_PADDR;
//   wire        gwct_PSEL;
//   // ... other signals ...
//
//   gwct_debug_bridge_n #(
//       .CLK_HZ(50_000_000),
//       .BAUD(115_200),
//       .NUM_APB_BUSES(1),
//       .ADDR_BITS(28)
//   ) gwct (
//       .clk(HCLK),
//       .rstn(hwRstn),
//       .uart_rx(GWCT_RX),
//       .uart_tx(GWCT_TX),
//       .PADDR(gwct_PADDR),
//       .PSEL(gwct_PSEL),
//       .PENABLE(gwct_PENABLE),
//       .PWRITE(gwct_PWRITE),
//       .PWDATA(gwct_PWDATA),
//       .PSTRB(gwct_PSTRB),
//       .PPROT(gwct_PPROT),
//       .PRDATA(slave_PRDATA),
//       .PREADY(slave_PREADY),
//       .PSLVERR(slave_PSLVERR)
//   );
//
// Example with 3 buses:
//   // Flattened arrays - bus signals concatenated
//   wire [32*3-1:0] gwct_PADDR;    // 3 x 32-bit = 96 bits
//   wire [3-1:0]    gwct_PSEL;     // 3 select bits
//   // Extract: bus0 = PADDR[31:0], bus1 = PADDR[63:32], bus2 = PADDR[95:64]
//
// Address Map (with ADDR_BITS=28, NUM_APB_BUSES=7):
//   Bus 0: 0x00000000 - 0x0FFFFFFF
//   Bus 1: 0x10000000 - 0x1FFFFFFF
//   Bus 2: 0x20000000 - 0x2FFFFFFF
//   ...
//
// =============================================================================

module gwct_debug_bridge_n #(
    parameter CLK_HZ = 50_000_000,
    parameter BAUD   = 115_200,
    parameter NUM_APB_BUSES = 1,    // Number of APB buses (1 to N)
    parameter ADDR_BITS = 28        // Address bits used for routing
                                     // Bus select = addr[31:ADDR_BITS]
                                     // 28 = 16 buses max (4 bits)
                                     // 24 = 256 buses max (8 bits)
)(
    input            clk,
    input            rstn,

    // UART interface
    input            uart_rx,
    output           uart_tx,

    // APB master interface - flattened arrays (Verilog-2001 compatible)
    // Each bus's signals are concatenated into wider bit vectors
    output [32*NUM_APB_BUSES-1:0] PADDR,    // NUM_APB_BUSES x 32-bit addresses
    output [NUM_APB_BUSES-1:0]    PSEL,     // NUM_APB_BUSES select signals
    output [NUM_APB_BUSES-1:0]    PENABLE,  // NUM_APB_BUSES enable signals
    output [NUM_APB_BUSES-1:0]    PWRITE,   // NUM_APB_BUSES write signals
    output [32*NUM_APB_BUSES-1:0] PWDATA,   // NUM_APB_BUSES x 32-bit write data
    output [4*NUM_APB_BUSES-1:0]  PSTRB,    // NUM_APB_BUSES x 4-bit strobe
    output [3*NUM_APB_BUSES-1:0]  PPROT,    // NUM_APB_BUSES x 3-bit prot
    input  [32*NUM_APB_BUSES-1:0] PRDATA,   // NUM_APB_BUSES x 32-bit read data
    input  [NUM_APB_BUSES-1:0]    PREADY,   // NUM_APB_BUSES ready signals
    input  [NUM_APB_BUSES-1:0]    PSLVERR   // NUM_APB_BUSES error signals
);

    // Calculate how many bits needed to represent NUM_APB_BUSES
    // Ensure at least 1 bit even for single bus to avoid [-1:0] wires
    function integer clog2(input integer value);
        begin
            if (value <= 1)
                clog2 = 1;  // At least 1 bit
            else begin
                clog2 = 0;
                while ((1 << clog2) < value)
                    clog2 = clog2 + 1;
            end
        end
    endfunction

    localparam BUS_SEL_BITS = clog2(NUM_APB_BUSES);

    // =========================================================================
    // UART and packet layers
    // =========================================================================
    wire [7:0] rx_data;
    wire       rx_valid;
    wire [7:0] tx_data;
    wire       tx_valid;
    wire       tx_ready;

    wire [31:0] cmd_addr;
    wire [31:0] cmd_wdata;
    wire        cmd_write;
    wire        cmd_valid;
    wire        cmd_ready;
    wire [31:0] cmd_rdata;
    wire        cmd_error;           // To gwct_packet (includes all errors)
    wire        apb_cmd_error;       // From gwct_apb_master (just APB errors)

    gwct_uart #(
        .CLK_HZ(CLK_HZ),
        .BAUD(BAUD)
    ) uart_inst (
        .clk      (clk),
        .rstn     (rstn),
        .rx       (uart_rx),
        .tx       (uart_tx),
        .rx_data  (rx_data),
        .rx_valid (rx_valid),
        .tx_data  (tx_data),
        .tx_valid (tx_valid),
        .tx_ready (tx_ready)
    );

    gwct_packet packet_inst (
        .clk        (clk),
        .rstn       (rstn),
        .rx_data    (rx_data),
        .rx_valid   (rx_valid),
        .tx_data    (tx_data),
        .tx_valid   (tx_valid),
        .tx_ready   (tx_ready),
        .cmd_addr   (cmd_addr),
        .cmd_wdata  (cmd_wdata),
        .cmd_write  (cmd_write),
        .cmd_valid  (cmd_valid),
        .cmd_ready  (cmd_ready),
        .cmd_rdata  (cmd_rdata),
        .cmd_error  (cmd_error)
    );

    // =========================================================================
    // Address decode - use upper bits to select bus
    // =========================================================================
    localparam UPPER_BITS = 32 - ADDR_BITS;
    
    wire [UPPER_BITS-1:0] bus_select_raw = cmd_addr[31:ADDR_BITS];
    wire [BUS_SEL_BITS-1:0] bus_select = (NUM_APB_BUSES == 1) ? 1'b0 : bus_select_raw[BUS_SEL_BITS-1:0];
    
    // Check if selected bus is valid
   // wire addr_decode_error = (bus_select_raw >= NUM_APB_BUSES);
    wire addr_decode_error = (NUM_APB_BUSES > 1) && (bus_select_raw >= NUM_APB_BUSES);

    // =========================================================================
    // APB master outputs (before routing to buses)
    // =========================================================================
    wire [31:0] master_PADDR;
    wire        master_PSEL;
    wire        master_PENABLE;
    wire        master_PWRITE;
    wire [31:0] master_PWDATA;
    wire [3:0]  master_PSTRB;
    wire [2:0]  master_PPROT;

    // =========================================================================
    // Route master signals to selected bus (flattened array indexing)
    // =========================================================================
    genvar i;
    generate
        for (i = 0; i < NUM_APB_BUSES; i = i + 1) begin : bus_routing
            // Each bus gets 32 bits of PADDR: PADDR[32*i +: 32]
            assign PADDR[32*i +: 32]  = master_PADDR;
            assign PSEL[i]            = master_PSEL && (bus_select == i) && !addr_decode_error;
            assign PENABLE[i]         = master_PENABLE && (bus_select == i) && !addr_decode_error;
            assign PWRITE[i]          = master_PWRITE;
            assign PWDATA[32*i +: 32] = master_PWDATA;
            assign PSTRB[4*i +: 4]    = master_PSTRB;
            assign PPROT[3*i +: 3]    = master_PPROT;
        end
    endgenerate

    // =========================================================================
    // Mux slave responses from selected bus (flattened array indexing)
    // =========================================================================
    wire [31:0] selected_PRDATA  = PRDATA[32*bus_select +: 32];
    wire        selected_PREADY  = PREADY[bus_select];
    wire        selected_PSLVERR = PSLVERR[bus_select];

    // =========================================================================
    // APB master FSM
    // =========================================================================
    // When there's an address decode error, we need to respond immediately
    // without going through the APB master FSM
    
    wire master_cmd_valid = cmd_valid && !addr_decode_error;
    
    // If decode error, respond immediately. Otherwise wait for APB master.
    reg addr_error_ready;
    always @(posedge clk or negedge rstn) begin
        if (!rstn)
            addr_error_ready <= 1'b0;
        else
            addr_error_ready <= cmd_valid && addr_decode_error;
    end
    
    wire apb_master_ready;
    assign cmd_ready = addr_error_ready || apb_master_ready;
    
    gwct_apb_master apb_master_inst (
        .clk        (clk),
        .rstn       (rstn),
        .cmd_addr   (cmd_addr),
        .cmd_wdata  (cmd_wdata),
        .cmd_write  (cmd_write),
        .cmd_valid  (master_cmd_valid),
        .cmd_ready  (apb_master_ready),
        .cmd_rdata  (cmd_rdata),
        .cmd_error  (apb_cmd_error),
        .PADDR      (master_PADDR),
        .PSEL       (master_PSEL),
        .PENABLE    (master_PENABLE),
        .PWRITE     (master_PWRITE),
        .PWDATA     (master_PWDATA),
        .PSTRB      (master_PSTRB),
        .PPROT      (master_PPROT),
        .PRDATA     (selected_PRDATA),
        .PREADY     (selected_PREADY),
        .PSLVERR    (selected_PSLVERR)
    );
    
    // Combine APB error with address decode error to send to packet layer
    assign cmd_error = apb_cmd_error || addr_decode_error;

endmodule