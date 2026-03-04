// ahb_sram.v
module ahb_sram #(
    parameter SIZE = 16384,                 // 16 KB
    parameter BASE_ADDR = 32'h8000_0000     // match your desired region
)(
    input  wire        HCLK,
    input  wire        HRESETn,

    // AHB-Lite slave interface
    input  wire [31:0] HADDR,
    input  wire        HWRITE,
    input  wire [2:0]  HSIZE,
    input  wire [1:0]  HTRANS,
    input  wire [31:0] HWDATA,
    input  wire        HSEL,                // chip select (internal or external)

    output reg  [31:0] HRDATA,
    output wire        HREADYOUT,            // always 1 when selected
    output reg  [1:0]  HRESP
);

    // Memory array (4096 x 32-bit)
    reg [31:0] mem [0:4095];

    // Byte lane write enables based on HSIZE and HADDR[1:0]
    wire [3:0] write_strobe;
    wire [11:0] word_addr = HADDR[13:2];    // 16 KB / 4 = 4096 words

    // Generate byte strobes for write (supports byte/halfword/word)
    assign write_strobe[0] = HWRITE && (HSIZE == 3'b000) ? (HADDR[1:0] == 2'b00) :
                              (HSIZE == 3'b001) ? (HADDR[1] == 1'b0) :
                              (HSIZE == 3'b010) ? 1'b1 : 1'b0;
    assign write_strobe[1] = HWRITE && (HSIZE == 3'b000) ? (HADDR[1:0] == 2'b01) :
                              (HSIZE == 3'b001) ? (HADDR[1] == 1'b0) :
                              (HSIZE == 3'b010) ? 1'b1 : 1'b0;
    assign write_strobe[2] = HWRITE && (HSIZE == 3'b000) ? (HADDR[1:0] == 2'b10) :
                              (HSIZE == 3'b001) ? (HADDR[1] == 1'b1) :
                              (HSIZE == 3'b010) ? 1'b1 : 1'b0;
    assign write_strobe[3] = HWRITE && (HSIZE == 3'b000) ? (HADDR[1:0] == 2'b11) :
                              (HSIZE == 3'b001) ? (HADDR[1] == 1'b1) :
                              (HSIZE == 3'b010) ? 1'b1 : 1'b0;

    // HREADYOUT: always ready when selected (zero wait states)
    assign HREADYOUT = HSEL ? 1'b1 : 1'b1;   // or tie to 1'b1, but keep for clarity

    // Read operation (synchronous)
    always @(posedge HCLK) begin
        if (HSEL && HTRANS[1] && !HWRITE)   // read transaction (NONSEQ or SEQ)
            HRDATA <= mem[word_addr];
    end

    // Write operation
    always @(posedge HCLK) begin
        if (HSEL && HTRANS[1] && HWRITE) begin
            if (write_strobe[0]) mem[word_addr][7:0]   <= HWDATA[7:0];
            if (write_strobe[1]) mem[word_addr][15:8]  <= HWDATA[15:8];
            if (write_strobe[2]) mem[word_addr][23:16] <= HWDATA[23:16];
            if (write_strobe[3]) mem[word_addr][31:24] <= HWDATA[31:24];
        end
    end

    // HRESP: always OKAY
    always @(posedge HCLK or negedge HRESETn) begin
        if (!HRESETn)
            HRESP <= 2'b00;
        else if (HSEL)
            HRESP <= 2'b00;   // OKAY
    end

endmodule