"""cocotb testbench for the FMC->wb-16 bridge + cheby `core` register file.

drives STM32 FMC multiplexed async cycles from python and checks:
  - magic   (RO, link/gateware check, 0xACE1)
  - fpga_id (RO, per-board target - driven here like a board top.v would)
  - version (RO)
  - scratch (RW round-trip)

run:  make        (icarus + cocotb)
"""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles
from cocotb.types import LogicArray

MAGIC      = 0xACE1
FPGA_GW2AR = 0x2018   # GW2AR-18, Tang Nano 20K
FPGA_GW5A  = 0x5025   # GW5A-25, Tang Primer 25K
VERSION    = 0x0001

# register word addresses (STM32 byte address = word * 2)
MAGIC_ADDR   = 0
FPGA_ID_ADDR = 1
SCRATCH_ADDR = 2
VERSION_ADDR = 3


def drive_ad(dut, value):
    dut.AD.value = value


def release_ad(dut):
    # high-Z so the DUT drives the bus during the read data phase
    dut.AD.value = LogicArray("Z" * 16)


async def do_reset(dut, fpga_id=FPGA_GW2AR):
    dut.NE1.value = 1
    dut.NOE.value = 1
    dut.NWE.value = 1
    dut.NADV.value = 1
    release_ad(dut)
    # RO register values - what a board's top.v hardwires
    dut.magic_i.value = MAGIC
    dut.fpga_id_i.value = fpga_id
    dut.version_i.value = VERSION
    dut.rst.value = 1
    await ClockCycles(dut.clk, 5)
    dut.rst.value = 0
    await ClockCycles(dut.clk, 5)


async def fmc_write(dut, addr, data):
    dut.NE1.value = 0
    # address phase
    dut.NADV.value = 0
    drive_ad(dut, addr)
    await ClockCycles(dut.clk, 6)
    dut.NADV.value = 1
    await ClockCycles(dut.clk, 4)
    # data phase
    drive_ad(dut, data)
    dut.NWE.value = 0
    await ClockCycles(dut.clk, 6)
    dut.NWE.value = 1            # rising edge -> bridge commits the wb write
    await ClockCycles(dut.clk, 6)
    release_ad(dut)
    dut.NE1.value = 1
    await ClockCycles(dut.clk, 6)


async def fmc_read(dut, addr):
    dut.NE1.value = 0
    # address phase
    dut.NADV.value = 0
    drive_ad(dut, addr)
    await ClockCycles(dut.clk, 6)
    dut.NADV.value = 1
    await ClockCycles(dut.clk, 2)
    release_ad(dut)             # bus turnaround
    await ClockCycles(dut.clk, 4)
    # data phase (bridge runs a wb read, then drives AD)
    dut.NOE.value = 0
    await ClockCycles(dut.clk, 10)
    val = int(dut.AD.value)
    dut.NOE.value = 1
    dut.NE1.value = 1
    await ClockCycles(dut.clk, 6)
    return val


@cocotb.test()
async def core_regs_test(dut):
    cocotb.start_soon(Clock(dut.clk, 20, unit="ns").start())
    await do_reset(dut, fpga_id=FPGA_GW2AR)

    val = await fmc_read(dut, MAGIC_ADDR)
    assert val == MAGIC, f"magic = 0x{val:04x}, expected 0x{MAGIC:04x}"

    val = await fmc_read(dut, FPGA_ID_ADDR)
    assert val == FPGA_GW2AR, f"fpga_id = 0x{val:04x}, expected 0x{FPGA_GW2AR:04x}"

    val = await fmc_read(dut, VERSION_ADDR)
    assert val == VERSION, f"version = 0x{val:04x}, expected 0x{VERSION:04x}"

    await fmc_write(dut, SCRATCH_ADDR, 0x1234)
    val = await fmc_read(dut, SCRATCH_ADDR)
    assert val == 0x1234, f"scratch read-back = 0x{val:04x}, expected 0x1234"

    await fmc_write(dut, SCRATCH_ADDR, 0xBEEF)
    val = await fmc_read(dut, SCRATCH_ADDR)
    assert val == 0xBEEF, f"scratch read-back = 0x{val:04x}, expected 0xBEEF"

    # per-board: same gateware, different fpga_id driven by top.v
    dut.fpga_id_i.value = FPGA_GW5A
    await ClockCycles(dut.clk, 2)
    val = await fmc_read(dut, FPGA_ID_ADDR)
    assert val == FPGA_GW5A, f"fpga_id (GW5A) = 0x{val:04x}, expected 0x{FPGA_GW5A:04x}"

    dut._log.info("all core register checks passed (incl. per-board fpga_id)")
