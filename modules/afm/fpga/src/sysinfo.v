module system_info (
    input         APBCLK,
    input         APBRESET,
    input  [31:0] PADDR,
    input         PSEL,
    input         PENABLE,
    input         PWRITE,
    input  [31:0] PWDATA,
    output [31:0] PRDATA,
    output        PREADY,
    output [1:0]  PSLVERR
);
    // Cheby-generated regs instance
    wire [31:0] prdata_cheby;
    wire        pready_cheby;

    sysinfo_regs u_regs (
        .pclk(APBCLK),
        .presetn(APBRESET),
        .paddr(PADDR[4:2]),   // word addressing
        .psel(PSEL),
        .pwrite(PWRITE),
        .penable(PENABLE),
        .pready(pready_cheby),
        .pwdata(PWDATA),
        .pstrb(4'b1111),
        .prdata(prdata_cheby),
        .pslverr(),

        // register values
        .magic_i(32'hDEADBEEF),
        .mfg_code_A_i(32'h476F7769),
        .mfg_code_B_i(32'h6E36304B),
        .version_patch_i(8'd0),
        .version_minor_i(8'd0),
        .version_major_i(16'd1),
        // cheby version stuff
        .cheby_version_patch_i(8'd0),
        .cheby_version_minor_i(8'd7),
        .cheby_version_major_i(16'd1)
    );

    assign PRDATA  = prdata_cheby;
    assign PREADY  = pready_cheby;
    assign PSLVERR = 2'b00;
endmodule
