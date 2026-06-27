# plain icarus verilog flow for the FMC slave testbench
#   make        -> build + run the testbench
#   make wave   -> run then open the waveform in gtkwave
#   make clean

SRC = ../rtl/fmc_mux_slave.v ../rtl/sine_lut.v
TB  = fmc_mux_slave_tb.v
OUT = fmc_tb

.PHONY: run wave clean
run: $(OUT)
	vvp $(OUT)

$(OUT): $(SRC) $(TB)
	iverilog -g2012 -o $(OUT) $(SRC) $(TB)

wave: run
	gtkwave fmc_tb.vcd

clean:
	rm -f $(OUT) fmc_tb.vcd
