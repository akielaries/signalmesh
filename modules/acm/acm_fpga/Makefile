# ACM FPGA builds
#
#   make BOARD=basys3 bit       build the bitstream (vivado, batch)
#   make BOARD=basys3 prog      load it over the onboard USB-JTAG (openFPGALoader)
#   make BOARD=basys3 gui       open the routed design in the vivado gui
#   make BOARD=basys3 project   create a .xpr project for the gui flow
#   make BOARD=mcb    bit       MAIN_COMPUTE_BOARD stub
#   make sync-xdc               refresh the MCB pins from signalmesh_CAD
#   make sim                    cocotb benches (sim/cocotb)
#   make clean
#
# gowin boards (gw2ar18, gw5a25) build in the gowin ide from boards/<board>/acm_fpga.gprj

BOARD    ?= basys3
VIVADO   ?= /tools/Xilinx/2025.2/Vivado/bin/vivado
LOADER   ?= openFPGALoader

board_dir_basys3 := boards/xc7a35t_basys3
board_dir_mcb    := boards/xc7a100t_mcb
loader_basys3    := -b basys3

BOARD_DIR := $(board_dir_$(BOARD))
OUT       := build/$(BOARD)
VFLAGS    := -mode batch -nojournal -log vivado.log -notrace

ifeq ($(BOARD_DIR),)
$(error unknown BOARD=$(BOARD), use basys3 or mcb (gowin boards build in the gowin ide))
endif

RTL := $(shell grep -v '^\s*\#' rtl.f) $(wildcard $(BOARD_DIR)/*.v $(BOARD_DIR)/*.xdc $(BOARD_DIR)/*.tcl)

.PHONY: bit prog gui project sync-xdc sim clean

bit: $(OUT)/top.bit

$(OUT)/top.bit: $(RTL) scripts/vivado/build.tcl scripts/vivado/common.tcl
	@mkdir -p $(OUT)
	cd $(OUT) && $(VIVADO) $(VFLAGS) -source $(CURDIR)/scripts/vivado/build.tcl \
	  -tclargs $(CURDIR)/$(BOARD_DIR) $(CURDIR)/$(OUT)

prog: $(OUT)/top.bit
	@if [ -z "$(loader_$(BOARD))" ]; then echo "no openFPGALoader board for $(BOARD)"; exit 1; fi
	$(LOADER) $(loader_$(BOARD)) $(OUT)/top.bit

gui: $(OUT)/top.bit
	cd $(OUT) && $(VIVADO) -nojournal -nolog $(CURDIR)/$(OUT)/post_route.dcp &

project:
	@mkdir -p $(OUT)
	cd $(OUT) && $(VIVADO) $(VFLAGS) -source $(CURDIR)/scripts/vivado/project.tcl \
	  -tclargs $(CURDIR)/$(BOARD_DIR) $(CURDIR)/$(OUT)

sync-xdc:
	scripts/sync_mcb_xdc.sh

sim:
	$(MAKE) -C sim/cocotb all

clean:
	rm -rf build
