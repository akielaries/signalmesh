//Copyright (C)2014-2026 GOWIN Semiconductor Corporation.
//All rights reserved.
//File Title: Timing Constraints file
//Tool Version: V1.9.12.01 
//Created Time: 2026-02-05 14:47:11
create_clock -name hclk -period 20 -waveform {0 10} [get_ports {HCLK}]
create_clock -name swd_clk -period 200 -waveform {0 100} [get_ports {JTAG_9_SWDCLK}]

set_clock_groups -exclusive -group [get_clocks {hclk}] -group [get_clocks {swd_clk}]