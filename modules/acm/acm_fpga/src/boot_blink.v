// blinker task to run on boot of FPGA

module boot_blinker (
  input wire HCLK,
  input wire HRST,
  output reg BOOT_LED
);

// 0.5s counter at 27MHz

reg [24:0] counter;
reg led_out;

always @(posedge HCLK or negedge HRST) begin
  if (!HRST) begin
    counter <= 25'd0;
    led_out <= 1'b0;
  end
  else begin
    if (counter == 25_000_000 - 1) begin
      counter <= 25'd0;
      led_out <= ~led_out;
    end 
    else begin
      counter <= counter + 1'b1;
    end
  end
end

assign BOOT_LED = led_out;

endmodule