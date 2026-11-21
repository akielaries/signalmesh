# Basys 3 (XC7A35T-1CPG236C) constraints for the ACM
#
# FMC wiring from the Nucleo-144 H755 (3.3 V, jumper wires):
#   pmod JA = AD[7:0], JB = AD[15:8], JC = control strobes
#   JC8 is reserved for FMC_CLK (sync mux mode), JC9/JC10 and JXADC are spare
# pmod pin numbering: 1-4 top row, 7-10 bottom row, 5/11 GND, 6/12 3V3

set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 4 [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]

# 100 MHz oscillator
set_property -dict {PACKAGE_PIN W5 IOSTANDARD LVCMOS33} [get_ports clk]
create_clock -period 10.000 -name sys_clk [get_ports clk]

# centre button = reset
set_property -dict {PACKAGE_PIN U18 IOSTANDARD LVCMOS33} [get_ports btnC]

# leds
set_property -dict {PACKAGE_PIN U16 IOSTANDARD LVCMOS33} [get_ports {led[0]}]
set_property -dict {PACKAGE_PIN E19 IOSTANDARD LVCMOS33} [get_ports {led[1]}]
set_property -dict {PACKAGE_PIN U19 IOSTANDARD LVCMOS33} [get_ports {led[2]}]
set_property -dict {PACKAGE_PIN V19 IOSTANDARD LVCMOS33} [get_ports {led[3]}]
set_property -dict {PACKAGE_PIN W18 IOSTANDARD LVCMOS33} [get_ports {led[4]}]
set_property -dict {PACKAGE_PIN U15 IOSTANDARD LVCMOS33} [get_ports {led[5]}]
set_property -dict {PACKAGE_PIN U14 IOSTANDARD LVCMOS33} [get_ports {led[6]}]
set_property -dict {PACKAGE_PIN V14 IOSTANDARD LVCMOS33} [get_ports {led[7]}]
set_property -dict {PACKAGE_PIN V13 IOSTANDARD LVCMOS33} [get_ports {led[8]}]
set_property -dict {PACKAGE_PIN V3  IOSTANDARD LVCMOS33} [get_ports {led[9]}]
set_property -dict {PACKAGE_PIN W3  IOSTANDARD LVCMOS33} [get_ports {led[10]}]
set_property -dict {PACKAGE_PIN U3  IOSTANDARD LVCMOS33} [get_ports {led[11]}]
set_property -dict {PACKAGE_PIN P3  IOSTANDARD LVCMOS33} [get_ports {led[12]}]
set_property -dict {PACKAGE_PIN N3  IOSTANDARD LVCMOS33} [get_ports {led[13]}]
set_property -dict {PACKAGE_PIN P1  IOSTANDARD LVCMOS33} [get_ports {led[14]}]
set_property -dict {PACKAGE_PIN L1  IOSTANDARD LVCMOS33} [get_ports {led[15]}]

# 7-seg anodes (active low), held off
set_property -dict {PACKAGE_PIN U2 IOSTANDARD LVCMOS33} [get_ports {an[0]}]
set_property -dict {PACKAGE_PIN U4 IOSTANDARD LVCMOS33} [get_ports {an[1]}]
set_property -dict {PACKAGE_PIN V4 IOSTANDARD LVCMOS33} [get_ports {an[2]}]
set_property -dict {PACKAGE_PIN W4 IOSTANDARD LVCMOS33} [get_ports {an[3]}]

# onboard USB-UART (FT2232 channel B), fpga -> host
set_property -dict {PACKAGE_PIN A18 IOSTANDARD LVCMOS33} [get_ports uart_tx]

# pmod JA: AD[7:0]
set_property -dict {PACKAGE_PIN J1 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[0]}]
set_property -dict {PACKAGE_PIN L2 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[1]}]
set_property -dict {PACKAGE_PIN J2 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[2]}]
set_property -dict {PACKAGE_PIN G2 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[3]}]
set_property -dict {PACKAGE_PIN H1 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[4]}]
set_property -dict {PACKAGE_PIN K2 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[5]}]
set_property -dict {PACKAGE_PIN H2 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[6]}]
set_property -dict {PACKAGE_PIN G3 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[7]}]

# pmod JB: AD[15:8]
set_property -dict {PACKAGE_PIN A14 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[8]}]
set_property -dict {PACKAGE_PIN A16 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[9]}]
set_property -dict {PACKAGE_PIN B15 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[10]}]
set_property -dict {PACKAGE_PIN B16 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[11]}]
set_property -dict {PACKAGE_PIN A15 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[12]}]
set_property -dict {PACKAGE_PIN A17 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[13]}]
set_property -dict {PACKAGE_PIN C15 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[14]}]
set_property -dict {PACKAGE_PIN C16 IOSTANDARD LVCMOS33} [get_ports {FMC_AD[15]}]

# pmod JC: control
set_property -dict {PACKAGE_PIN K17 IOSTANDARD LVCMOS33} [get_ports FMC_NE1]
set_property -dict {PACKAGE_PIN M18 IOSTANDARD LVCMOS33} [get_ports FMC_NADV]
set_property -dict {PACKAGE_PIN N17 IOSTANDARD LVCMOS33} [get_ports FMC_NOE]
set_property -dict {PACKAGE_PIN P18 IOSTANDARD LVCMOS33} [get_ports FMC_NWE]
set_property -dict {PACKAGE_PIN L17 IOSTANDARD LVCMOS33} [get_ports FMC_NWAIT]
# JC8 M19 = FMC_CLK (reserved), JC9 P17 / JC10 R18 spare

# strobes idle high when the STM32 is unplugged or in reset
set_property PULLUP TRUE [get_ports {FMC_NE1 FMC_NADV FMC_NOE FMC_NWE}]

# the bus is asynchronous: every input is 2-FF synchronised in fmc_wb_bridge and
# the STM32 timing is set slow enough to cover it, so no io timing applies
set_false_path -from [get_ports {FMC_AD[*] FMC_NE1 FMC_NADV FMC_NOE FMC_NWE btnC}]
set_false_path -to [get_ports {FMC_AD[*] FMC_NWAIT led[*] an[*] uart_tx}]
