# FPGA targets

The ACM gateware (`modules/acm/acm_fpga`) builds for four boards. The shared RTL in `rtl/`
is vendor-neutral (no primitives), so each target only adds a board layer: `top.v`, pin
constraints, timing constraints, clock divider and its `fpga_id`.

| target | role | status |
|---|---|---|
| Tang Nano 20K (GW2AR-18C) | first bring-up board, Nucleo-H755 over FMC | working |
| Tang Primer 25K (GW5A-25) | second Gowin target, same datapath | working |
| Basys 3 (XC7A35T) | Xilinx stand-in until the main board is fabbed | builds, timing met, untested on hardware |
| MAIN_COMPUTE_BOARD (XC7A100T) | nominal board, see `signalmesh_CAD/boards/MAIN_COMPUTE_BOARD` | in layout, stub top builds |

## device resources

Numbers from the vendor datasheets (Gowin DS226/DS1103, Xilinx DS180). Verify against the
current datasheet before relying on a margin.

| | GW2AR-18C | GW5A-25 | XC7A35T | XC7A100T |
|---|---|---|---|---|
| part on board | GW2AR-LV18QN88C8/I7 | GW5A-LV25MG121NC1/I0 | XC7A35T-1CPG236C | XC7A100T-2FGG484I |
| LUTs | 20,736 LUT4 | 23,040 LUT4 | 20,800 LUT6 | 63,400 LUT6 |
| flip-flops | 15,552 | 23,040 | 41,600 | 126,800 |
| distributed RAM | 41 Kb | 180 Kb | 400 Kb | 1,188 Kb |
| block RAM | 828 Kb (46 x 18 Kb) | 1,008 Kb (56 x 18 Kb) | 1,800 Kb (50 x 36 Kb) | 4,860 Kb (135 x 36 Kb) |
| multipliers / DSP | 48 x 18x18 | 28 x 27x18 DSP | 90 DSP48E1 | 240 DSP48E1 |
| PLL / clock tiles | 2 PLL | 6 PLL | 5 CMT (MMCM + PLL) | 6 CMT (MMCM + PLL) |
| serial transceivers | - | - | not used on Basys 3 | 4 GTP, 6.25 Gb/s |
| ADC | - | yes (on-die) | XADC 12-bit 1 MSPS | XADC 12-bit 1 MSPS |
| in-package memory | 64 Mbit SDRAM | - | - | - |
| speed grade | C8/I7 | C1/I0 | -1 | -2 |

A LUT6 holds roughly 1.6x the logic of a LUT4, so the 35T is about 1.5x a GW5A-25 in practice
and the 100T is about 3x the 35T. The DSP count is what limits voice count and FX.

## board features

| | Tang Nano 20K | Tang Primer 25K | Basys 3 | MAIN_COMPUTE_BOARD |
|---|---|---|---|---|
| fabric clock | 27 MHz | 50 MHz | 50 MHz (MMCM from the 100 MHz oscillator) | 100 MHz SYSCLK + MMCM |
| audio clock | divided from 27 MHz | divided from 50 MHz | divided from 100 MHz | 24.576 / 22.5792 MHz XOs |
| `TICK_DIV` for 48 kHz | 562 | 1042 | 1042 | MCLK based |
| `fpga_id` | 0x2018 | 0x5025 | 0x7035 (planned) | 0x7100 (planned) |
| external memory | 64 Mbit SDRAM (in package) | - | - | DDR3L 512 MB x16, 1.6 GB/s |
| config flash | SPI flash on board | SPI flash on module | 32 Mbit Quad SPI | none: STM32 loads it over SelectMAP |
| programming / debug | onboard USB JTAG + UART | onboard USB JTAG + UART (dock) | onboard USB JTAG + UART (FT2232) | JTAG headers (Xilinx 2x7, Digilent 1x6) |
| user IO for the bus | pin headers | Pmods / headers on the dock | 4 Pmods, 32 pins (JA JB JC JXADC) | dedicated bank 14 to the STM32 |
| audio out | via STM32 DAC | via STM32 DAC | via STM32 DAC (or Pmod I2S) | CS4344 DAC + OPA1656, TDM backplane |
| extras | HDMI, TF card, RGB LED | - | 16 sw, 16 LED, 5 btn, 7-seg, VGA | SFP x2, VITA 57.1 HPC, Pmod x2 |

## STM32 link per target

| target | MCU | FMC mode | pins |
|---|---|---|---|
| Tang Nano 20K | Nucleo-144 H755 | 16-bit muxed AD, async, NADV latch | ~21 |
| Tang Primer 25K | Nucleo-144 H755 | 16-bit muxed AD, async | ~21 |
| Basys 3 | Nucleo-144 H755 | 16-bit muxed AD (async, then sync with FMC_CLK) | 21-22 on 3 Pmods |
| MAIN_COMPUTE_BOARD | STM32H755XIH6 on board | 32-bit sync NOR/PSRAM, A0-A18, NBL0-3, 120-125 MHz | ~60 |

The 32-bit bus does not fit on the Basys 3, even with rework: 32 Pmod pins plus the 14 VGA lines
is still short, and the VGA lines go through the resistor-ladder DAC resistors. The Basys 3 runs the
same 16-bit bus as the Gowin boards. The 32-bit sync bridge is verified in cocotb until the main
board exists.

## toolchains

| target | synth / P&R | program |
|---|---|---|
| Tang Nano 20K, Tang Primer 25K | Gowin EDA (`gw_ide`) | `openFPGALoader -b tangnano20k` / `-b tangprimer25k` |
| Basys 3 | Vivado ML Standard (free) | `openFPGALoader -b basys3`, or Vivado hardware manager |
| MAIN_COMPUTE_BOARD | Vivado ML Standard (MIG needed for DDR3) | STM32 SelectMAP loader, or JTAG |

### installing Vivado

Installed: Vivado ML Standard 2025.2, Artix-7 devices only, at `/tools/Xilinx` (about 52 GB),
from the offline installer `FPGAs_AdaptiveSoCs_Unified_SDI_2025.2_1114_2157`. Shell setup:

```
source /tools/Xilinx/2025.2/Vivado/settings64.sh
```

Cable drivers (once, as root), then replug the board:

```
cd /tools/Xilinx/2025.2/Vivado/data/xicom/cable_drivers/lin64/install_script/install_drivers
sudo ./install_drivers
```

What each installer product is, and whether this project needs it:

| product | what it is | needed |
|---|---|---|
| Vivado | synthesis, P&R, bitstream, IP (MIG, clocking wizard), hardware manager, `hw_server` | yes, ML Standard edition |
| Vitis | full software platform (HLS, accelerators, embedded) | no |
| Vitis Embedded Development | compiler and debugger for soft cores (MicroBlaze, Arm DesignStart M1/M3) | later, only for the soft core on the main board |
| BootGen | builds boot images for Zynq / Versal | no (Artix-7 bitstreams come from Vivado) |
| Lab Edition | programming and debug only, no synthesis | no (included in Vivado) |
| Hardware Server | standalone `hw_server` for a remote programming host | no (included in Vivado) |
| Power Design Manager | power estimation for UltraScale+ / Versal | no (use Vivado `report_power`) |

To add a product later without reinstalling, run `xsetup -b Add` from the installer, or pick
"Add Design Tools or Devices" in the GUI.

## build layout

```
modules/acm/acm_fpga/
  rtl/                    shared, vendor-neutral RTL
  rtl.f                   shared RTL file list for the vivado flow
  boards/
    gw2ar18/              Tang Nano 20K, gowin ide project
    gw5a25/               Tang Primer 25K, gowin ide project
    xc7a35t_basys3/       top.v, basys3.xdc, board.tcl
    xc7a100t_mcb/         top.v (stub), mcb_pins.xdc (synced), board.tcl
  scripts/
    vivado/build.tcl      non-project batch build
    vivado/project.tcl    creates a .xpr for the gui
    sync_mcb_xdc.sh       pulls mcb_pins.xdc from signalmesh_CAD
  Makefile
  sim/cocotb/             benches
```

`board.tcl` holds the part, top module, constraint files and board-only sources. A new
Xilinx target is a new `boards/<part>_<board>/` folder plus one line in the Makefile.

Basys 3 Pmod wiring to the Nucleo-144 H755 (pins from `modules/apm/tests/test_fmc_fpga.c`):

| signal | Nucleo | Basys 3 |
|---|---|---|
| AD0-AD3 | PD14 PD15 PD0 PD1 | JA1-JA4 |
| AD4-AD7 | PE7 PE8 PE9 PE10 | JA7-JA10 |
| AD8-AD11 | PE11 PE12 PE13 PE14 | JB1-JB4 |
| AD12-AD15 | PE15 PD8 PD9 PD10 | JB7-JB10 |
| NE1 | PC7 | JC1 |
| NADV (NL) | PB7 | JC2 |
| NOE | PD4 | JC3 |
| NWE | PD5 | JC4 |
| NWAIT | PC6 | JC7 |
| FMC_CLK (reserved) | PD3 | JC8 |
| GND | GND | Pmod pin 5 or 11 |

## building and loading

From `modules/acm/acm_fpga`:

```
make BOARD=basys3 bit       # bitstream in build/basys3/top.bit
make BOARD=basys3 prog      # load over USB-JTAG with openFPGALoader (no vivado needed)
make BOARD=mcb bit          # MAIN_COMPUTE_BOARD stub
make sync-xdc               # after the CAD pin plan changes
```

Each build writes to `build/<board>/` (gitignored):

| file | what it is |
|---|---|
| `top.bit` | bitstream |
| `post_synth.dcp`, `post_route.dcp` | design checkpoints, open these in the gui |
| `reports/timing.rpt` | timing summary, WNS and the worst paths |
| `reports/util.rpt`, `util_hier.rpt` | resource use, flat and per module |
| `reports/io.rpt` | every pin with its standard and bank |
| `reports/clocks.rpt`, `power.rpt`, `drc.rpt` | clock buffers, power estimate, design rule checks |
| `vivado.log` | full log |

The build exits with an error if timing is not met.

## viewing designs in Vivado

Two ways in. The scripted build is the source of truth; the gui is for looking.

**Routed checkpoint (fastest).** `make BOARD=basys3 gui`, or start `vivado` and use
File > Checkpoint > Open on `build/basys3/post_route.dcp`. This opens the placed and routed
design with no project. Useful views from the Window menu or the Flow Navigator:

| to see | where |
|---|---|
| placement and routing on the die | Window > Device |
| pin assignments and banks | Window > I/O Ports, or Layout > I/O Planning |
| timing and the worst paths | Reports > Timing > Report Timing Summary, then double-click a path |
| resource use per module | Reports > Report Utilization, then the Hierarchy view |
| schematic of a module or path | select a cell or path, press F4 |
| clock network | Reports > Report Clock Networks |

Selecting a path in the timing report highlights it in the Device and Schematic views.

**Project (full gui flow).** `make BOARD=basys3 project`, then File > Project > Open on
`build/basys3/proj/acm_xc7a35t_basys3.xpr`. Sources point at the repo files. Here the
Flow Navigator runs Synthesis, Implementation and Generate Bitstream, and you can open the
elaborated RTL schematic (RTL Analysis > Open Elaborated Design) before synthesis.
Changes you make to sources in the gui land in the repo files, but project settings do not
feed back into the scripted build, so put lasting changes in `board.tcl` or the XDC.

**Hardware manager.** Open Hardware Manager > Open Target > Auto Connect, then Program Device
with `build/basys3/top.bit`. This needs the cable drivers installed. `make prog` does the same
without the gui.

**Simulation.** The cocotb benches stay the main simulation flow (`make sim`). Vivado's
simulator (XSim) is available from the project's Flow Navigator if a waveform view of a
Xilinx primitive such as the MMCM is needed.
