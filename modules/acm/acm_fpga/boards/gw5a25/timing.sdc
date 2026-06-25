create_clock -name clk_50M -period 20 -waveform {0 10} [get_ports {HCLK}]
