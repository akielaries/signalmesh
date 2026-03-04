//Copyright (C)2014-2025 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: IP file
//Tool Version: V1.9.12.01
//IP Version: 1.0
//Part Number: GW5AT-LV60PG484AC2/I1
//Device: GW5AT-60
//Device Version: B
//Created Time: Thu Feb  5 15:37:44 2026

module Gowin_CLKDIV (clkout, hclkin, resetn);

output clkout;
input hclkin;
input resetn;

wire gw_gnd;

assign gw_gnd = 1'b0;

CLKDIV clkdiv_inst (
    .CLKOUT(clkout),
    .HCLKIN(hclkin),
    .RESETN(resetn),
    .CALIB(gw_gnd)
);

defparam clkdiv_inst.DIV_MODE = "8";

endmodule //Gowin_CLKDIV
