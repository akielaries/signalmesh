# cocotb (python) flow for fmc_mux_slave
#
# setup once:   pip install cocotb        (icarus verilog is the simulator)
# run:          make                      (builds + runs test_fmc_mux_slave.py)
# waveform:     make WAVES=1 && gtkwave dump.vcd   (icarus dumps if the TB does)
# clean:        make clean

SIM           ?= icarus
TOPLEVEL_LANG ?= verilog

VERILOG_SOURCES = $(PWD)/../../rtl/fmc_mux_slave.v $(PWD)/../../rtl/sine_lut.v

# top-level RTL module and the python test module (test_fmc_mux_slave.py)
COCOTB_TOPLEVEL     = fmc_mux_slave
COCOTB_TEST_MODULES = test_fmc_mux_slave

include $(shell cocotb-config --makefiles)/Makefile.sim
