// top.v
// Main top-level module for the project.
// - Instantiates PLL for stable clocking (RECOMMENDED).
// - Instantiates the user's 'uart' module to handle all UART (TX/RX) functionality and LED control.

module top (
    // --- System Interface ---
    input  wire bank1_3v3_xtal_in,   // 27 MHz clock input
    input  wire bank3_1v8_sys_rst,   // Active-low reset input

    // --- LED Interface ---
    output wire bank2_3v3_red_led,  // Off-board red LED
    output wire [5:0] bank3_1v8_led,    // On-board LEDs (driven by uart module)

    // --- UART Interface ---
    output wire bank2_3v3_uart_tx,   // UART Transmit pin
    input  wire bank2_3v3_uart_rx,   // UART Receive pin
    input  wire bank3_1v8_btn1       // User button 1 (connected to uart module)
);

    // --- Internal Signals & Wires ---
    wire sys_clk;
    wire rst_n;
    wire pll_clk; // Main clock for the design (from PLL)


    // --- Clocking and Reset ---
    assign sys_clk = bank1_3v3_xtal_in;
    assign rst_n = bank3_1v8_sys_rst; // Global reset for top-level

    Gowin_rPLL rpll_inst (
        .clkout(pll_clk),
        .clkin(sys_clk)
    );


    // Instantiate the user's combined UART module
    uart #(
        .DELAY_FRAMES(234) // 27,000,000 / 115200
    ) uart_inst (
        .clk(sys_clk),          // Use PLL clock
        .uart_rx(bank2_3v3_uart_rx), // Connect top-level RX pin
        .uart_tx(bank2_3v3_uart_tx), // Connect top-level TX pin
        .led(bank3_1v8_led),    // Connect uart's LEDs to top-level onboard LEDs
        .btn1(bank3_1v8_btn1)   // Connect top-level button
    );
    
    assign bank2_3v3_red_led = 1'b0;

endmodule