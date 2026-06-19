"""cocotb testbench for fmc_mux_slave.

drives the STM32 FMC multiplexed async cycles from python and checks the
ID handshake, scratch write/read-back, and the LED_CTRL register.

run:  pip install cocotb   then   make
"""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles
from cocotb.types import LogicArray   # older cocotb: from cocotb.binary import BinaryValue

ID_VALUE = 0xACE1

# register word addresses (STM32 byte address = word * 2)
ID_ADDR      = 0
SCRATCH_ADDR = 1
LED_ADDR     = 2


def drive_ad(dut, value):
    dut.AD.value = value


def release_ad(dut):
    # high-Z so the DUT can drive the bus during the read data phase
    dut.AD.value = LogicArray("Z" * 16)
    # older cocotb: dut.AD.value = BinaryValue("z" * 16)


async def do_reset(dut):
    dut.NE1.value = 1
    dut.NOE.value = 1
    dut.NWE.value = 1
    dut.NADV.value = 1
    release_ad(dut)
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
    dut.NWE.value = 1            # rising edge -> slave commits
    await ClockCycles(dut.clk, 4)
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
    release_ad(dut)             # turn the bus around
    await ClockCycles(dut.clk, 4)
    # data phase
    dut.NOE.value = 0
    await ClockCycles(dut.clk, 8)
    val = int(dut.AD.value)    # DUT drives all 16 bits here, no Z
    dut.NOE.value = 1
    dut.NE1.value = 1
    await ClockCycles(dut.clk, 6)
    return val


@cocotb.test()
async def fmc_mux_slave_test(dut):
    cocotb.start_soon(Clock(dut.clk, 20, unit="ns").start())
    await do_reset(dut)

    val = await fmc_read(dut, ID_ADDR)
    assert val == ID_VALUE, f"ID = 0x{val:04x}, expected 0x{ID_VALUE:04x}"

    val = await fmc_read(dut, SCRATCH_ADDR)
    assert val == 0x0000, f"scratch initial = 0x{val:04x}"

    await fmc_write(dut, SCRATCH_ADDR, 0x1234)
    val = await fmc_read(dut, SCRATCH_ADDR)
    assert val == 0x1234, f"scratch read-back = 0x{val:04x}"

    await fmc_write(dut, LED_ADDR, 0x002A)
    val = await fmc_read(dut, LED_ADDR)
    assert val == 0x002A, f"led_ctrl read-back = 0x{val:04x}"

    led = int(dut.led_ctrl.value)
    assert led == 0x002A, f"led_ctrl output = 0x{led:04x}"

    dut._log.info("all FMC mux slave checks passed")
