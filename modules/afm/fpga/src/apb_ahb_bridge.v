// =============================================================================
// apb2ahb_bridge.v — APB to AHB-Lite bridge
//
// Allows an APB master (like GWCT) to access AHB slaves (like SRAM).
//
// Protocol conversion:
//   APB transaction:  SETUP (1 cycle) + ACCESS (N cycles until PREADY)
//   AHB transaction:  ADDRESS (1 cycle) + DATA (1 cycle, or wait states)
//
// Usage:
//   Connect GWCT (APB master) to the APB side
//   Connect AHB SRAM to the AHB side
//   Bridge handles protocol conversion automatically
//
// Example:
//   gwct_debug_bridge_n → apb2ahb_bridge → ahb_sram
//
// Address mapping:
//   The bridge passes addresses through unchanged
//   Use address decode in your top level to select this bridge
//
// =============================================================================

module apb2ahb_bridge (
    input            clk,
    input            rstn,

    // APB slave interface (from GWCT or other APB master)
    input  [31:0]    PADDR,
    input            PSEL,
    input            PENABLE,
    input            PWRITE,
    input  [31:0]    PWDATA,
    input  [3:0]     PSTRB,      // byte strobes (optional, not used in basic impl)
    output reg [31:0] PRDATA,
    output reg       PREADY,
    output reg       PSLVERR,

    // AHB master interface (to AHB slaves like SRAM)
    output reg [31:0] HADDR,
    output reg        HWRITE,
    output reg [2:0]  HSIZE,
    output reg [1:0]  HTRANS,
    output reg [31:0] HWDATA,
    output reg        HSEL,
    input  [31:0]    HRDATA,
    input            HREADYOUT,
    input  [1:0]     HRESP
);

    // FSM states
    localparam IDLE    = 2'b00;
    localparam ADDR    = 2'b01;  // AHB address phase
    localparam DATA    = 2'b10;  // AHB data phase
    
    reg [1:0] state;
    reg       latched_write;
    reg [31:0] latched_wdata;

    // Decode PSTRB to HSIZE (simplified - assumes word aligned)
    // In production, you'd want more sophisticated size decode
    function [2:0] pstrb_to_hsize(input [3:0] strb);
        casez (strb)
            4'b0001, 4'b0010, 4'b0100, 4'b1000: pstrb_to_hsize = 3'b000;  // byte
            4'b0011, 4'b1100:                    pstrb_to_hsize = 3'b001;  // halfword
            4'b1111:                             pstrb_to_hsize = 3'b010;  // word
            default:                             pstrb_to_hsize = 3'b010;  // word (default)
        endcase
    endfunction

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            state        <= IDLE;
            HADDR        <= 32'h0;
            HWRITE       <= 1'b0;
            HSIZE        <= 3'b010;   // word
            HTRANS       <= 2'b00;    // IDLE
            HWDATA       <= 32'h0;
            HSEL         <= 1'b0;
            PRDATA       <= 32'h0;
            PREADY       <= 1'b0;
            PSLVERR      <= 1'b0;
            latched_write <= 1'b0;
            latched_wdata <= 32'h0;
        end else begin
            case (state)
                
                // =========================================================
                // IDLE: Wait for APB transaction
                // =========================================================
                IDLE: begin
                    HTRANS  <= 2'b00;   // IDLE
                    HSEL    <= 1'b0;
                    PREADY  <= 1'b0;
                    PSLVERR <= 1'b0;
                    
                    if (PSEL && !PENABLE) begin
                        // APB SETUP phase - prepare AHB address phase
                        state       <= ADDR;
                        HADDR       <= PADDR;
                        HWRITE      <= PWRITE;
                        HSIZE       <= pstrb_to_hsize(PSTRB);
                        HTRANS      <= 2'b10;   // NONSEQ (start of transaction)
                        HSEL        <= 1'b1;
                        latched_write <= PWRITE;
                        latched_wdata <= PWDATA;
                    end
                end

                // =========================================================
                // ADDR: AHB address phase
                // =========================================================
                ADDR: begin
                    // AHB address phase completes in 1 cycle
                    // Move to data phase
                    state  <= DATA;
                    HTRANS <= 2'b00;   // IDLE (no more transfers)
                    
                    // For writes, present data on HWDATA now
                    if (latched_write) begin
                        HWDATA <= latched_wdata;
                    end
                end

                // =========================================================
                // DATA: AHB data phase (wait for HREADYOUT)
                // =========================================================
                DATA: begin
                    if (HREADYOUT) begin
                        // AHB transaction complete
                        if (!latched_write) begin
                            // Read: capture data
                            PRDATA <= HRDATA;
                        end
                        
                        // Check for error
                        PSLVERR <= (HRESP != 2'b00);  // OKAY = 00, ERROR = 01
                        
                        // Signal APB transaction complete
                        PREADY <= 1'b1;
                        
                        // Return to IDLE
                        state  <= IDLE;
                        HSEL   <= 1'b0;
                    end
                    // else: wait for HREADYOUT (AHB slave inserting wait states)
                end

                default: state <= IDLE;
            endcase
        end
    end

endmodule