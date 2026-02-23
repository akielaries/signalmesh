// top.v
// Main top-level module for the project.
// - Instantiates PLL for stable clocking of the blink module (RECOMMENDED).
// - Uses raw system clock for the uart module (as requested).
// - Instantiates 'blink.v' for red LED and onboard LEDs 0 & 1.
// - Instantiates the user's 'uart' module for UART (TX/RX) and onboard LEDs 2-5.

module top (
    // --- System Interface ---
    input  wire bank1_3v3_xtal_in,   // 27 MHz clock input
    input  wire bank3_1v8_sys_rst,   // Active-low reset input

    // --- LED Interface ---
    output wire bank2_3v3_red_led,  // Off-board red LED
    output wire [5:0] bank3_1v8_led,    // On-board LEDs

    // --- UART Interface ---
    output wire bank2_3v3_uart_tx,   // UART Transmit pin
    input  wire bank2_3v3_uart_rx,   // UART Receive pin
    input  wire bank3_1v8_btn1       // User button 1 (connected to uart module)
);

    // --- Internal Signals & Wires ---
    wire sys_clk;
    wire rst_n;
    wire pll_clk; // Main clock for the blink module (from PLL)

    // Connections for blink module
    wire offboard_led_from_blink;
    wire [5:0] onboard_leds_from_blink;

    // Internal wire for uart module's LED output
    wire [5:0] uart_led_internal; // New internal wire to receive LEDs from uart_inst

    // --- Clocking and Reset ---
    assign sys_clk = bank1_3v3_xtal_in; // Raw system clock
    assign rst_n = bank3_1v8_sys_rst;   // Global reset for top-level

    // PLL Instantiation - RECOMMENDED for stable clocking for blink module
    Gowin_rPLL rpll_inst (
        .clkout(pll_clk), // PLL output for blink module
        .clkin(sys_clk)   // PLL input from raw system clock
    );

    // --- Module Instantiations ---

    // Instantiate the blinker module
    blink blink_inst (
        .clk(pll_clk), // Clocked by stable PLL output
        .rst_n(rst_n),
        .o_red_led(offboard_led_from_blink),
        .o_onboard_leds(onboard_leds_from_blink)
    );

    // Instantiate the user's combined UART module
    uart #(
        .DELAY_FRAMES(234) // 27,000,000 / 115200
    ) uart_inst (
        .clk(sys_clk),          // Clocked by raw system clock, as requested
        .uart_rx(bank2_3v3_uart_rx), // Connect top-level RX pin
        .uart_tx(bank2_3v3_uart_tx), // Connect top-level TX pin
        .led(uart_led_internal),// Connect uart's LEDs to internal wire
        .btn1(bank3_1v8_btn1)   // Connect top-level button
    );
    
    // --- Final Output Assignments ---
    // Off-board red LED is driven by the blink module
    assign bank2_3v3_red_led = offboard_led_from_blink;

    // On-board LEDs: Combine outputs from blink and uart modules
    // Assuming blink drives LED0 and LED1, and uart drives LED2-5
    // Note: If uart.v also controls LED0/1, uart.v's assignment will take precedence.
    assign bank3_1v8_led = {
        uart_led_internal[5:2],           // LEDs 5,4,3,2 from uart module's internal LEDs
        onboard_leds_from_blink[1:0]  // LEDs 1,0 from blink module
    };

endmodule