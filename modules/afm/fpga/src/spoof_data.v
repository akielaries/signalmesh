// This module contains a small ROM with the spoofed PID/CID values.
module spoof_data (
    input wire [11:0] addr,
    output wire [31:0] data
);

    reg [31:0] rom [0:7];

    initial begin
        // PIDR0-3
        rom[0] = 32'h00000091;
        rom[1] = 32'h000000b4;
        rom[2] = 32'h0000000b;
        rom[3] = 32'h00000000;
        // CIDR0-3 (Preamble)
        rom[4] = 32'h0d;
        rom[5] = 32'h00;
        rom[6] = 32'h05;
        rom[7] = 32'hb1;
    end

    assign data = rom[addr[4:2]];

endmodule
