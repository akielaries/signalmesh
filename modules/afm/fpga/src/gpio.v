module gpio (
    input         APBCLK,
    input         APBRESET,
    input  [31:0] PADDR,
    input         PSEL,
    input         PENABLE,
    input         PWRITE,
    input  [31:0] PWDATA,

    output [31:0] PRDATA,
    output        PREADY,
    output        PSLVERR,

    // optional external signals
    output [31:0] gpio_out,
    input  [31:0] gpio_stat
);

    wire [31:0] prdata_cheby;
    wire        pready_cheby;

    gpio_regs u_regs (
        .pclk    (APBCLK),
        .presetn (APBRESET),

        // Cheby generated: only bit [2]
        .paddr   (PADDR[2:2]),

        .psel    (PSEL),
        .pwrite  (PWRITE),
        .penable (PENABLE),
        .pready  (pready_cheby),

        .pwdata  (PWDATA),
        .pstrb   (4'b1111),

        .prdata  (prdata_cheby),
        .pslverr (),

        // registers
        .out_o   (gpio_out),
        .stat_i  (gpio_stat)
    );

    assign PRDATA  = prdata_cheby;
    assign PREADY  = pready_cheby;
    assign PSLVERR = 1'b0;

endmodule
