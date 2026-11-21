# vivado board config, sourced by scripts/vivado/*.tcl
set PART   xc7a100tfgg484-2
set TOP    top
set XDC    [list mcb_pins.xdc]
set SRCS   [list top.v]

# mcb_pins.xdc covers the whole board (DDR3, VITA, SFP, GTP, audio). the stub top
# only has some of those ports, so "no ports matched" is expected for now
set_msg_config -id {Vivado 12-584} -new_severity {WARNING}
set_msg_config -id {Common 17-55}  -new_severity {WARNING}
set_msg_config -id {Vivado 12-4739} -new_severity {WARNING}
