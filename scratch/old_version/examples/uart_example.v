`default_nettype none

module uart_hello_world #(
  parameter CLK_FREQ = 27000000, // Clock frequency in Hz
  parameter BAUD_RATE = 115200   // Desired baud rate
)(
  input wire bank1_3v3_xtal_in,
  input wire bank2_3v3_uart_rx,
  output wire bank2_3v3_uart_tx,
  output wire [5:0] bank3_1v8_led
);

  // Calculate the baud rate divisor based on the clock frequency and baud rate
  localparam BAUD_DIV = CLK_FREQ / BAUD_RATE;

  // The message to be transmitted.
  // The length is 14 characters.
  localparam [14*8-1:0] MESSAGE = "Hello, world!\r\n";
  localparam MESSAGE_LEN = 14;

  reg [7:0] tx_data_reg;
  reg enable_tx_reg;
  wire tx_done_wire;

  // Instantiate the UART module
  uart #(
    .BAUD_DIV(BAUD_DIV)
  ) uart_instance (
    .clk(bank1_3v3_xtal_in),
    .uart_rx(bank2_3v3_uart_rx),
    .uart_tx(bank2_3v3_uart_tx),
    .led(bank3_1v8_led),           // Connect the LED output
    .enable_tx(enable_tx_reg),
    .tx_data(tx_data_reg),
    .tx_done(tx_done_wire)
  );

  // State machine to send the message character by character
  reg [2:0] state = S_IDLE;
  localparam S_IDLE = 0;
  localparam S_SEND_CHAR = 1;
  localparam S_WAIT_DONE = 2;
  localparam S_DELAY = 3;

  reg [$clog2(MESSAGE_LEN):0] char_index = 0;
  reg [23:0] delay_counter = 0;

  always @(posedge bank1_3v3_xtal_in) begin
    case (state)
      // Idle state, waits for a bit before starting transmission
      S_IDLE: begin
        enable_tx_reg <= 1'b0;
        char_index <= 0;
        state <= S_SEND_CHAR;
      end

      // Load the character into the tx_data register and enable transmission
      S_SEND_CHAR: begin
        tx_data_reg <= MESSAGE[char_index*8 +: 8];
        enable_tx_reg <= 1'b1;
        state <= S_WAIT_DONE;
      end

      // Wait for the UART to signal that transmission of the byte is complete
      S_WAIT_DONE: begin
        enable_tx_reg <= 1'b0; // De-assert enable_tx after one cycle
        if (tx_done_wire) begin
          if (char_index == MESSAGE_LEN - 1) begin
            // If the whole message is sent, go to the delay state
            state <= S_DELAY;
            delay_counter <= 0;
          end else begin
            // Otherwise, send the next character
            char_index <= char_index + 1;
            state <= S_SEND_CHAR;
          end
        end
      end

      // State to add a delay between sending messages
      S_DELAY: begin
        if (delay_counter == 5000000) begin
          state <= S_IDLE; // Restart the process
        end else begin
          delay_counter <= delay_counter + 1;
        end
      end

    endcase
  end

endmodule
