module ddr3_cpu_wrapper (
    input clk,
    input rstn,
    
    // APB slave interface
    input [31:0] apb_paddr,
    input apb_psel,
    input apb_penable,
    input apb_pwrite,
    input [31:0] apb_pwdata,
    output reg [31:0] apb_prdata,
    output apb_pready,
    
    // DDR3 arbiter interface
    output reg cpu_req,
    output reg cpu_we,
    output reg [27:0] cpu_addr,
    output reg [127:0] cpu_wdata,
    input [127:0] cpu_rdata,
    input cpu_ack,
    input cpu_busy,
    
    input init_complete  // Changed from output to input
);

    // Register addresses
    localparam ADDR_RESERVED  = 5'h00;
    localparam ADDR_WR_ADDR   = 5'h01;
    localparam ADDR_WR_DATA0  = 5'h02;
    localparam ADDR_WR_DATA1  = 5'h03;
    localparam ADDR_WR_DATA2  = 5'h04;
    localparam ADDR_WR_DATA3  = 5'h05;
    localparam ADDR_RD_ADDR   = 5'h06;
    localparam ADDR_RD_EN     = 5'h07;
    localparam ADDR_RD_DATA0  = 5'h08;
    localparam ADDR_RD_DATA1  = 5'h09;
    localparam ADDR_RD_DATA2  = 5'h0A;
    localparam ADDR_RD_DATA3  = 5'h0B;
    localparam ADDR_INIT      = 5'h0C;
    localparam ADDR_WR_EN     = 5'h0D;
    
    reg [27:0] wr_addr_reg;
    reg [127:0] wr_data_reg;
    reg [27:0] rd_addr_reg;
    reg [127:0] rd_data_reg;
    reg wr_en_reg;
    reg rd_en_reg;
    
    wire [4:0] reg_addr = apb_paddr[6:2];
    
    // APB writes
    always @(posedge clk or negedge rstn) begin
        if (!rstn) begin
            wr_addr_reg <= 28'h0;
            wr_data_reg <= 128'h0;
            rd_addr_reg <= 28'h0;
            wr_en_reg <= 1'b0;
            rd_en_reg <= 1'b0;
            cpu_req <= 1'b0;
            cpu_we <= 1'b0;
            cpu_addr <= 28'h0;
            cpu_wdata <= 128'h0;
        end else begin
            // Clear enables and request after ack
            if (cpu_ack) begin
                cpu_req <= 1'b0;
                wr_en_reg <= 1'b0;
                rd_en_reg <= 1'b0;
            end
            
            // Latch read data when it arrives
            if (cpu_ack && !cpu_we) begin
                rd_data_reg <= cpu_rdata;
            end
            
            // Handle APB writes
            if (apb_psel && apb_penable && apb_pwrite) begin
                case (reg_addr)
                    ADDR_WR_ADDR:  wr_addr_reg <= apb_pwdata[27:0];
                    ADDR_WR_DATA0: wr_data_reg[31:0] <= apb_pwdata;
                    ADDR_WR_DATA1: wr_data_reg[63:32] <= apb_pwdata;
                    ADDR_WR_DATA2: wr_data_reg[95:64] <= apb_pwdata;
                    ADDR_WR_DATA3: wr_data_reg[127:96] <= apb_pwdata;
                    ADDR_RD_ADDR:  rd_addr_reg <= apb_pwdata[27:0];
                    
                    ADDR_WR_EN: begin
                        if (apb_pwdata[0] && !cpu_req) begin
                            cpu_req <= 1'b1;
                            cpu_we <= 1'b1;
                            cpu_addr <= wr_addr_reg;
                            cpu_wdata <= wr_data_reg;
                            wr_en_reg <= 1'b1;
                        end
                    end
                    
                    ADDR_RD_EN: begin
                        if (apb_pwdata[0] && !cpu_req) begin
                            cpu_req <= 1'b1;
                            cpu_we <= 1'b0;
                            cpu_addr <= rd_addr_reg;
                            rd_en_reg <= 1'b1;
                        end
                    end
                endcase
            end
        end
    end
    
    // APB reads
    always @(*) begin
        apb_prdata = 32'h0;
        
        if (apb_psel) begin
            case (reg_addr)
                ADDR_WR_ADDR:  apb_prdata = {4'h0, wr_addr_reg};
                ADDR_WR_DATA0: apb_prdata = wr_data_reg[31:0];
                ADDR_WR_DATA1: apb_prdata = wr_data_reg[63:32];
                ADDR_WR_DATA2: apb_prdata = wr_data_reg[95:64];
                ADDR_WR_DATA3: apb_prdata = wr_data_reg[127:96];
                ADDR_RD_ADDR:  apb_prdata = {4'h0, rd_addr_reg};
                ADDR_RD_DATA0: apb_prdata = rd_data_reg[31:0];
                ADDR_RD_DATA1: apb_prdata = rd_data_reg[63:32];
                ADDR_RD_DATA2: apb_prdata = rd_data_reg[95:64];
                ADDR_RD_DATA3: apb_prdata = rd_data_reg[127:96];
                ADDR_INIT:     apb_prdata = {31'h0, init_complete};  // Pass through input
                ADDR_WR_EN:    apb_prdata = {31'h0, wr_en_reg};
                ADDR_RD_EN:    apb_prdata = {31'h0, rd_en_reg};
                default:       apb_prdata = 32'h0;
            endcase
        end
    end
    
    assign apb_pready = 1'b1;

endmodule