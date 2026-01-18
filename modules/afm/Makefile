#YOSYS=yosys

#FAMILY= GW1N-9C

#BLOCK=

blinky:
	yosys -p "read_verilog blinky.v; synth_gowin -top blinky -json blinky.json"
	nextpnr-himbaechel --json blinky.json --write pnrblinky.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=afm.cst
	gowin_pack -d GW1N-9C -o pack.fs pnrblinky.json

uart:
	yosys -p "read_verilog uart.v uart_test.v; synth_gowin -top uart_test -json uart.json"
	nextpnr-himbaechel --json uart.json --write pnruart.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=acm.cst
	gowin_pack -d GW1N-9C -o pack.fs pnruart.json

pll:
	yosys -p "read_verilog pll_example.v; synth_gowin -top pll_example -json pll_example.json"
	nextpnr-himbaechel --json pll_example.json --write pnrpll_example.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=afm.cst
	gowin_pack -d GW1N-9C -o pack.fs pnrpll_example.json

clk_div:
	yosys -p "read_verilog clk_div_example.v; synth_gowin -top clk_div_example -json clk_div_example.json"
	nextpnr-himbaechel --json clk_div_example.json --write pnrclk_div_example.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=afm.cst
	gowin_pack -d GW1N-9C -o pack.fs pnrclk_div_example.json

waveform:
	yosys -p "read_verilog sine_wave_gen.v; synth_gowin -top sine_wave_gen -json sine_wave_gen.json"
	nextpnr-himbaechel --json sine_wave_gen.json --write pnrsine_wave_gen.json --freq 27 --device GW1NR-LV9QN88PC6/I5 --vopt family=GW1N-9C --vopt cst=afm.cst
	gowin_pack -d GW1N-9C -o pack.fs pnrsine_wave_gen.json

uart_sim:
	iverilog -o sim_uart uart.v uart_tb.v
	vvp sim_uart

uart_wave:
	gtkwave uart.vcd

flash:
	../../tools/fpgaloader.py -f pack.fs -b tangnano9k

clean:
	rm -f *.fs *.json
