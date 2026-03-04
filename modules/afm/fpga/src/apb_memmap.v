module apb_memmap (
    input        APBCLK,
    input        APBRESET,
    input [31:0] PADDR,
    input        PSEL,
    input        PENABLE,
    input        PWRITE,
    input [31:0] PWDATA,
    output [31:0] PRDATA,
    output       PREADY,
    output       PSLVERR
);

    // i have no idea how to make this cleaner but these are the offsets
    // of the peripherals of the APB1 section starting @ 0x6000_0000
    localparam SYSINFO_BASE = 20'h00000;
    localparam SYSINFO_SIZE = 20'h00014;

    localparam GPIO_BASE    = 20'h00020;
    localparam GPIO_SIZE    = 20'h00008;


    // sub-block wires
    wire [31:0] sysinfo_prdata;
    wire        sysinfo_pready;
    wire [31:0] gpio_prdata;
    wire        gpio_pready;

    // decode address hits
    wire sysinfo_sel = PSEL && (PADDR[19:0] < 20'h14);
    wire gpio_sel    = PSEL && (PADDR[19:0] >= 20'h20 && PADDR[19:0] < 20'h30);
    
    // instantiate sub-blocks
    system_info sysinfo_inst (
        .APBCLK(APBCLK),
        .APBRESET(APBRESET),
        .PADDR(PADDR),
        .PSEL(PSEL & sysinfo_sel),
        .PENABLE(PENABLE),
        .PWRITE(PWRITE),
        .PWDATA(PWDATA),
        .PRDATA(sysinfo_prdata),
        .PREADY(sysinfo_pready),
        .PSLVERR()
    );

    wire [31:0] gpio_out;
    wire [31:0] gpio_stat = 32'h00C0FFEE;

    gpio gpio_inst (
        .APBCLK   (APBCLK),
        .APBRESET (APBRESET),

        .PADDR    (PADDR),
        .PSEL     (PSEL & gpio_sel),
        .PENABLE  (PENABLE),
        .PWRITE   (PWRITE),
        .PWDATA   (PWDATA),

        .PRDATA   (gpio_prdata),
        .PREADY   (gpio_pready),
        .PSLVERR  (),

        .gpio_out (gpio_out),
        .gpio_stat(gpio_stat)
    );

    // Route back to APB bus
    assign PRDATA =
        sysinfo_sel ? sysinfo_prdata :
        gpio_sel    ? gpio_prdata :
                      32'h00000000;

    assign PREADY =
        sysinfo_sel ? sysinfo_pready :
        gpio_sel    ? gpio_pready :
                      1'b1;

    //assign PSLVERR = (PSEL && !(sysinfo_sel || gpio_sel));
    assign PSLVERR = 1'b0;

endmodule
