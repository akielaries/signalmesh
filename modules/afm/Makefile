# Makefile for the 'afm' module
# This Makefile uses a simple, explicit rule for each design.

# --- Toolchain ---
YOSYS      = yosys
NEXTPNR    = nextpnr-himbaechel
GOWIN_PACK = gowin_pack

# --- Gowin Device ---
DEVICE  = GW1NR-LV9QN88PC6/I5
FAMILY  = GW1N-9C
FREQ    = 27 # Input clock frequency in MHz
CST_FILE = afm.cst

# --- All Verilog sources needed for most designs ---
# This makes the rules less repetitive.
V_SOURCES = rtl/top.v cores/pll_core.v
YOSYS_FLAGS = -Iinclude

# --- Build Targets ---

.PHONY: all top pll_example blinky flash clean

# Default target
all: top

# Target to build the main 'top' module
top: build/top.bit

build/top.bit: $(V_SOURCES) rtl/top.v
	@echo "### Building top module..."
	@mkdir -p build
	$(YOSYS) -p "read_verilog $(YOSYS_FLAGS) $(V_SOURCES); synth_gowin -top top -json build/top.json"
	$(NEXTPNR) --json build/top.json --write build/top_pnr.json --freq $(FREQ) --device $(DEVICE) --vopt family=$(FAMILY) --vopt cst=$(CST_FILE)
	$(GOWIN_PACK) -d $(FAMILY) -o $@ build/top_pnr.json

# Target to build the 'pll_example'
pll_example: build/pll_example.bit

build/pll_example.bit: examples/pll_example.v cores/pll_core.v
	@echo "### Building pll_example..."
	@mkdir -p build
	$(YOSYS) -p "read_verilog $(YOSYS_FLAGS) cores/pll_core.v examples/pll_example.v; synth_gowin -top pll_example -json build/pll_example.json"
	$(NEXTPNR) --json build/pll_example.json --write build/pll_example_pnr.json --freq $(FREQ) --device $(DEVICE) --vopt family=$(FAMILY) --vopt cst=$(CST_FILE)
	$(GOWIN_PACK) -d $(FAMILY) -o $@ build/pll_example_pnr.json

# Target to build the 'blinky' example
blinky: build/blinky.bit

build/blinky.bit: examples/blinky.v
	@echo "### Building blinky example..."
	@mkdir -p build
	$(YOSYS) -p "read_verilog $(YOSYS_FLAGS) examples/blinky.v; synth_gowin -top blinky -json build/blinky.json"
	$(NEXTPNR) --json build/blinky.json --write build/blinky_pnr.json --freq $(FREQ) --device $(DEVICE) --vopt family=$(FAMILY) --vopt cst=$(CST_FILE)
	$(GOWIN_PACK) -d $(FAMILY) -o $@ build/blinky_pnr.json


# --- Utility Targets ---

# Generic flash target - MUST specify the file to flash
# Example: make flash FILE=build/top.bit
flash:
	@if [ -z "$(FILE)" ]; then \
		echo "ERROR: Please specify the file to flash. Example: make flash FILE=build/top.bit"; \
		exit 1; \
	fi
	@echo "### Flashing $(FILE)..."
	../../tools/fpgaloader.py -f $(FILE) -b tangnano9k

clean:
	@echo "### Cleaning build artifacts..."
	rm -rf build *.log
