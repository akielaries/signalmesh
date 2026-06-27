# cocotb test flow - one tab-completable target per bench:
#
#   make fmc      # FMC->wb bridge + cheby `core` register map
#   make dds      # polyphonic DDS oscillator bank
#   make acm      # full datapath: FMC -> core+audio regs -> DDS -> sample
#   make mux      # legacy fmc_mux_slave (being retired)
#   make all      # run every bench in turn
#   make          # default: fmc
#   make clean    # remove all build dirs + results
#
# (uppercase aliases FMC/DDS/MUX work too.) the bench is chosen by TARGET=,
# which the named targets set and which propagates through cocotb's internal
# recursive make (MAKECMDGOALS does NOT - that's why this is split this way).
# each bench builds in its own sim_build_<name>/ so switches never go stale.

SIM           ?= icarus
TOPLEVEL_LANG ?= verilog

ifeq ($(TARGET),)
# ===== top level: user-facing named targets (recurse with TARGET set) =====
.DEFAULT_GOAL := fmc
.PHONY: fmc FMC dds DDS acm ACM mux MUX all clean cleanall

fmc FMC:
	@$(MAKE) TARGET=fmc sim
dds DDS:
	@$(MAKE) TARGET=dds sim
acm ACM:
	@$(MAKE) TARGET=acm sim
mux MUX:
	@$(MAKE) TARGET=mux sim
all:
	@$(MAKE) TARGET=fmc sim
	@$(MAKE) TARGET=dds sim
	@$(MAKE) TARGET=acm sim
	@$(MAKE) TARGET=mux sim
clean cleanall:
	rm -rf sim_build_* results.xml __pycache__

else
# ===== TARGET set: pick the config + hand off to cocotb's `sim` =====
ifeq ($(TARGET),dds)
  VERILOG_SOURCES = $(PWD)/../../rtl/dds_synth.v $(PWD)/../../rtl/sine_lut.v
  COCOTB_TOPLEVEL     = dds_synth
  COCOTB_TEST_MODULES = test_dds
else ifeq ($(TARGET),acm)
  VERILOG_SOURCES = $(PWD)/acm_tb.v \
                    $(PWD)/../../rtl/acm_top.v \
                    $(PWD)/../../rtl/fmc_wb_bridge.v \
                    $(PWD)/../../rtl/core_regs.v \
                    $(PWD)/../../rtl/audio_regs.v \
                    $(PWD)/../../rtl/dds_synth.v \
                    $(PWD)/../../rtl/sine_lut.v
  COCOTB_TOPLEVEL     = acm_tb
  COCOTB_TEST_MODULES = test_acm
else ifeq ($(TARGET),mux)
  VERILOG_SOURCES = $(PWD)/../../rtl/fmc_mux_slave.v $(PWD)/../../rtl/sine_lut.v
  COCOTB_TOPLEVEL     = fmc_mux_slave
  COCOTB_TEST_MODULES = test_fmc_mux_slave
else
  VERILOG_SOURCES = $(PWD)/fmc_wb_top.v \
                    $(PWD)/../../rtl/fmc_wb_bridge.v \
                    $(PWD)/../../rtl/core_regs.v
  COCOTB_TOPLEVEL     = fmc_wb_top
  COCOTB_TEST_MODULES = test_core_regs
endif

SIM_BUILD = sim_build_$(TARGET)

include $(shell cocotb-config --makefiles)/Makefile.sim

endif
