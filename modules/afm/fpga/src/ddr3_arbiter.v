// =============================================================================
// ddr3_arbiter.v
//
// Clocked by ddr_clk (DDR3 controller's clk_out).
//
// Key timing requirement for Gowin DDR3 1/4-rate controller:
//   For WRITE: cmd_en and wr_data_en MUST be asserted in the SAME clock cycle.
//     CMD_PHASE waits for both cmd_ready AND wr_data_rdy, then issues both
//     cmd_en and wr_data_en simultaneously in CMD_ISSUE.
//   For READ: cmd_en is issued when cmd_ready is high, then rd_data_valid
//     is awaited in RD_WAIT.
//
// CPU interface signals (cpu_req/we/addr/wdata) arrive from the APB1PCLK
// domain.  cpu_req is synchronized internally with a 2-FF chain; the data
// signals are stable long before req is asserted so they cross safely.
// cpu_ack is a level signal: it stays high until cpu_req_sync falls so the
// APB1PCLK-domain consumer has enough time to see it.
//
// RTL interface is in the same ddr_clk domain, no synchronization needed.
// rtl_ack is also level-based to prevent the arbiter from spuriously
// re-triggering on the still-high req after DONE.
// =============================================================================

module ddr3_arbiter (
    input clk,      // must be ddr_clk (DDR3 controller clk_out)
    input rstn,

    // CPU interface  (req/we/addr/wdata sourced from APB1PCLK domain)
    input         cpu_req,
    input         cpu_we,
    input  [27:0] cpu_addr,
    input [127:0] cpu_wdata,
    output reg [127:0] cpu_rdata,
    output reg    cpu_ack,      // level: held until cpu_req deasserts
    output        cpu_busy,

    // RTL interface  (same ddr_clk domain)
    input         rtl_req,
    input         rtl_we,
    input  [27:0] rtl_addr,
    input [127:0] rtl_wdata,
    output reg [127:0] rtl_rdata,
    output reg    rtl_ack,      // level: held until rtl_req deasserts
    output        rtl_busy,

    // DDR3 controller interface  (same ddr_clk domain)
    output reg  [2:0] ddr_cmd,
    output reg        ddr_cmd_en,
    input             ddr_cmd_ready,
    output reg [27:0] ddr_addr,
    output reg [127:0] ddr_wr_data,
    output reg        ddr_wr_data_en,
    output reg        ddr_wr_data_end,
    input             ddr_wr_data_rdy,
    input      [127:0] ddr_rd_data,
    input             ddr_rd_data_valid,
    input             ddr_rd_data_end
);

    localparam CMD_WRITE = 3'b000;
    localparam CMD_READ  = 3'b001;

    // States
    localparam IDLE      = 3'd0;
    localparam CMD_PHASE = 3'd1;  // waiting for cmd_ready (+ wr_data_rdy for writes)
    localparam CMD_ISSUE = 3'd2;  // cmd_en=1 (+ wr_data_en for writes) this cycle
    localparam RD_WAIT   = 3'd3;  // waiting for rd_data_valid
    localparam DONE      = 3'd4;
    localparam DONE_ACK  = 3'd5;  // hold ack until requester drops req

    reg [2:0] state;
    reg is_cpu;

    // 2-FF synchronizer: cpu_req (APB1PCLK) → ddr_clk
    reg cpu_req_s1, cpu_req_s2;
    always @(posedge clk or negedge rstn) begin
        if (!rstn) {cpu_req_s2, cpu_req_s1} <= 2'b00;
        else begin
            cpu_req_s1 <= cpu_req;
            cpu_req_s2 <= cpu_req_s1;
        end
    end
    wire cpu_req_sync = cpu_req_s2;

    assign cpu_busy = (state != IDLE) && is_cpu;
    assign rtl_busy = (state != IDLE) && !is_cpu;

    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            state           <= IDLE;
            ddr_cmd         <= 3'b0;
            ddr_cmd_en      <= 1'b0;
            ddr_addr        <= 28'h0;
            ddr_wr_data     <= 128'h0;
            ddr_wr_data_en  <= 1'b0;
            ddr_wr_data_end <= 1'b0;
            cpu_ack         <= 1'b0;
            rtl_ack         <= 1'b0;
            cpu_rdata       <= 128'h0;
            rtl_rdata       <= 128'h0;
            is_cpu          <= 1'b0;
        end else begin
            case (state)

                IDLE: begin
                    ddr_cmd_en      <= 1'b0;
                    ddr_wr_data_en  <= 1'b0;
                    ddr_wr_data_end <= 1'b0;

                    if (cpu_req_sync) begin         // CPU has priority
                        is_cpu       <= 1'b1;
                        ddr_addr     <= cpu_addr;
                        ddr_cmd      <= cpu_we ? CMD_WRITE : CMD_READ;
                        if (cpu_we) ddr_wr_data <= cpu_wdata;
                        state        <= CMD_PHASE;
                    end else if (rtl_req) begin
                        is_cpu       <= 1'b0;
                        ddr_addr     <= rtl_addr;
                        ddr_cmd      <= rtl_we ? CMD_WRITE : CMD_READ;
                        if (rtl_we) ddr_wr_data <= rtl_wdata;
                        state        <= CMD_PHASE;
                    end
                end

                // For WRITE: wait for both cmd_ready AND wr_data_rdy.
                // For READ:  wait for cmd_ready only.
                // Issue cmd_en (+ wr_data for write) simultaneously in next state.
                CMD_PHASE: begin
                    if (ddr_cmd == CMD_WRITE) begin
                        if (ddr_cmd_ready && ddr_wr_data_rdy) begin
                            ddr_cmd_en      <= 1'b1;
                            ddr_wr_data_en  <= 1'b1;
                            ddr_wr_data_end <= 1'b1;
                            state           <= CMD_ISSUE;
                        end
                    end else begin
                        if (ddr_cmd_ready) begin
                            ddr_cmd_en <= 1'b1;
                            state      <= CMD_ISSUE;
                        end
                    end
                end

                // cmd_en (and wr_data_en for writes) is high THIS cycle.
                // Clear all control pulses and transition to next stage.
                CMD_ISSUE: begin
                    ddr_cmd_en      <= 1'b0;
                    ddr_wr_data_en  <= 1'b0;
                    ddr_wr_data_end <= 1'b0;
                    // Writes complete here; reads wait for data.
                    state <= (ddr_cmd == CMD_WRITE) ? DONE : RD_WAIT;
                end

                RD_WAIT: begin
                    if (ddr_rd_data_valid) begin
                        if (is_cpu) cpu_rdata <= ddr_rd_data;
                        else        rtl_rdata <= ddr_rd_data;
                        state <= DONE;
                    end
                end

                DONE: begin
                    if (is_cpu) cpu_ack <= 1'b1;
                    else        rtl_ack <= 1'b1;
                    state <= DONE_ACK;
                end

                // Keep ack asserted until the requesting master drops its req.
                // For CPU: use the synced req (APB1PCLK→ddr_clk).
                //   The ack stays high long enough for the APB1PCLK sync chain
                //   to reliably capture it.
                // For RTL: same ddr_clk domain; req falls one cycle after the
                //   master sees ack, so we'll see it fall one cycle later.
                DONE_ACK: begin
                    if (is_cpu) begin
                        if (!cpu_req_sync) begin
                            cpu_ack <= 1'b0;
                            state   <= IDLE;
                        end
                    end else begin
                        if (!rtl_req) begin
                            rtl_ack <= 1'b0;
                            state   <= IDLE;
                        end
                    end
                end

                default: state <= IDLE;
            endcase
        end
    end

endmodule
