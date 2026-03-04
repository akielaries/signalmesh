// =============================================================================
// gwct_apb_master.v — APB master FSM for GWCT debug access
//
// Drives the APB3 protocol:
//   SETUP  phase: assert PSEL, PADDR, PWRITE, PWDATA (one cycle)
//   ACCESS phase: assert PENABLE, wait for PREADY (one or more cycles)
//
// Interfaces:
//   cmd_*    — from gwct_packet (command to execute)
//   APB_*    — to the APB bus mux / slave
// =============================================================================

module gwct_apb_master (
    input            clk,
    input            rstn,

    // Command interface (from gwct_packet)
    input  [31:0]    cmd_addr,
    input  [31:0]    cmd_wdata,
    input            cmd_write,
    input            cmd_valid,     // pulse: start a transaction
    output reg       cmd_ready,     // pulse: transaction complete
    output reg [31:0] cmd_rdata,   // data returned on read
    output reg       cmd_error,     // PSLVERR was asserted

    // APB master outputs → bus
    output reg [31:0] PADDR,
    output reg        PSEL,
    output reg        PENABLE,
    output reg        PWRITE,
    output reg [31:0] PWDATA,
    output reg [3:0]  PSTRB,
    output reg [2:0]  PPROT,

    // APB slave inputs ← bus
    input  [31:0]    PRDATA,
    input            PREADY,
    input            PSLVERR
);

    localparam ST_IDLE   = 2'd0,
               ST_SETUP  = 2'd1,   // PSEL=1 PENABLE=0 — setup phase
               ST_ACCESS = 2'd2,   // PSEL=1 PENABLE=1 — access phase, wait PREADY
               ST_DONE   = 2'd3;   // one-cycle done, deassert everything

    reg [1:0] state;

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            state      <= ST_IDLE;
            PADDR      <= 0;
            PSEL       <= 0;
            PENABLE    <= 0;
            PWRITE     <= 0;
            PWDATA     <= 0;
            PSTRB      <= 4'hF;     // all byte lanes enabled
            PPROT      <= 3'b000;   // normal, non-secure, data
            cmd_ready  <= 0;
            cmd_rdata  <= 0;
            cmd_error  <= 0;
        end else begin
            cmd_ready <= 0;     // default deasserted

            case (state)

                ST_IDLE: begin
                    PSEL    <= 0;
                    PENABLE <= 0;
                    if (cmd_valid) begin
                        // latch command and move to SETUP
                        PADDR   <= cmd_addr;
                        PWRITE  <= cmd_write;
                        PWDATA  <= cmd_wdata;
                        PSTRB   <= 4'hF;
                        PSEL    <= 1;
                        PENABLE <= 0;
                        state   <= ST_SETUP;
                    end
                end

                ST_SETUP: begin
                    // One full clock of setup phase, then assert PENABLE
                    PENABLE <= 1;
                    state   <= ST_ACCESS;
                end

                ST_ACCESS: begin
                    // Wait for slave to assert PREADY
                    if (PREADY) begin
                        cmd_rdata <= PRDATA;
                        cmd_error <= PSLVERR;
                        PSEL      <= 0;
                        PENABLE   <= 0;
                        state     <= ST_DONE;
                    end
                    // else stay here — slave is inserting wait states
                end

                ST_DONE: begin
                    cmd_ready <= 1;     // pulse ready to packet layer
                    state     <= ST_IDLE;
                end

            endcase
        end
    end

endmodule