#YOSYS=yosys

#FAMILY= GW1N-9C

#BLOCK=

uart:
	yosys -p "read_verilog uart.v uart_test.v; synth_gowin -top uart_test -json uart.json"
	nextpnr-himbaechel --json uart.json --write pnruart.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=tangnano9k_uart.cst
	gowin_pack -d GW1N-9C -o pack.fs pnruart.json

uart_sim:
	iverilog -o sim_uart uart.v uart_tb.v
	vvp sim_uart

uart_wave:
	gtkwave uart.vcd

flash:
	openFPGALoader -v -b tangnano9k pack.fs

clean:
	rm -f *.fs *.json
