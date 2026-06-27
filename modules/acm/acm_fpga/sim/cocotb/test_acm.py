"""cocotb testbench for acm_top - the full FPGA datapath over FMC.

drives STM32 FMC cycles and checks the whole chain end to end:
  - core identity reachable at 0x00 (magic, fpga_id)
  - audio voice regs reachable + RW at 0x80 (through the address decode)
  - 32-bit freq spans two 16-bit words
  - writing a gated voice makes the DDS produce a (varying) sample, read back
    from the audio `sample` register
  - clearing the gate returns silence

run:  make acm

word addresses (FMC byte offset / 2):
  core  magic   0x00       core  fpga_id 0x01
  audio base    0x40 (byte 0x80)
  audio sample  0x42       voice0 freq 0x60/0x61   voice0 ctrl 0x62
"""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles
from cocotb.types import LogicArray

MAGIC          = 0xACE1
FPGA_ID        = 0x2018
VERSION        = 0x0001

MAGIC_ADDR     = 0x00
FPGA_ID_ADDR   = 0x01
SAMPLE_ADDR    = 0x42
VOICE0_FREQ_HI = 0x60    # cheby is big-endian across the two words:
VOICE0_FREQ_LO = 0x61    #   low addr = freq[31:16], high addr = freq[15:0]
VOICE0_CTRL    = 0x62

# voice ctrl: gate(b0) | wave(b3:1) | level(b15:8)
def voice_ctrl(gate, wave, level):
    return (gate & 1) | ((wave & 7) << 1) | ((level & 0xFF) << 8)


def to_signed(v, bits=16):
    v &= (1 << bits) - 1
    return v - (1 << bits) if (v & (1 << (bits - 1))) else v


def drive_ad(dut, value):
    dut.FMC_AD.value = value


def release_ad(dut):
    dut.FMC_AD.value = LogicArray("Z" * 16)


async def do_reset(dut):
    dut.FMC_NE1.value = 1
    dut.FMC_NOE.value = 1
    dut.FMC_NWE.value = 1
    dut.FMC_NADV.value = 1
    release_ad(dut)
    dut.magic_i.value = MAGIC
    dut.fpga_id_i.value = FPGA_ID
    dut.version_i.value = VERSION
    dut.rst.value = 1
    await ClockCycles(dut.clk, 5)
    dut.rst.value = 0
    await ClockCycles(dut.clk, 5)


async def fmc_write(dut, addr, data):
    dut.FMC_NE1.value = 0
    dut.FMC_NADV.value = 0
    drive_ad(dut, addr)
    await ClockCycles(dut.clk, 6)
    dut.FMC_NADV.value = 1
    await ClockCycles(dut.clk, 4)
    drive_ad(dut, data)
    dut.FMC_NWE.value = 0
    await ClockCycles(dut.clk, 6)
    dut.FMC_NWE.value = 1
    await ClockCycles(dut.clk, 6)
    release_ad(dut)
    dut.FMC_NE1.value = 1
    await ClockCycles(dut.clk, 6)


async def fmc_read(dut, addr):
    dut.FMC_NE1.value = 0
    dut.FMC_NADV.value = 0
    drive_ad(dut, addr)
    await ClockCycles(dut.clk, 6)
    dut.FMC_NADV.value = 1
    await ClockCycles(dut.clk, 2)
    release_ad(dut)
    await ClockCycles(dut.clk, 4)
    dut.FMC_NOE.value = 0
    await ClockCycles(dut.clk, 10)
    val = int(dut.FMC_AD.value)
    dut.FMC_NOE.value = 1
    dut.FMC_NE1.value = 1
    await ClockCycles(dut.clk, 6)
    return val


@cocotb.test()
async def acm_test(dut):
    cocotb.start_soon(Clock(dut.clk, 20, unit="ns").start())
    await do_reset(dut)

    # core identity reachable at 0x00
    assert (v := await fmc_read(dut, MAGIC_ADDR)) == MAGIC, f"magic=0x{v:04x}"
    assert (v := await fmc_read(dut, FPGA_ID_ADDR)) == FPGA_ID, f"fpga_id=0x{v:04x}"

    # core scratch RW through the decode (write path, word 2 = byte 0x04)
    await fmc_write(dut, 0x02, 0xBEEF)
    assert (v := await fmc_read(dut, 0x02)) == 0xBEEF, f"core scratch=0x{v:04x}"

    # audio voice regs reachable + RW at 0x80 (through the decode)
    ctrl = voice_ctrl(gate=1, wave=1, level=0xFF)   # gated saw, full level
    await fmc_write(dut, VOICE0_CTRL, ctrl)
    assert (v := await fmc_read(dut, VOICE0_CTRL)) == ctrl, f"voice0 ctrl=0x{v:04x}"

    # 32-bit freq spans two words (big-endian), round-trips
    FREQ = 0x0080_0000
    await fmc_write(dut, VOICE0_FREQ_HI, (FREQ >> 16) & 0xFFFF)
    await fmc_write(dut, VOICE0_FREQ_LO, FREQ & 0xFFFF)
    hi = await fmc_read(dut, VOICE0_FREQ_HI)
    lo = await fmc_read(dut, VOICE0_FREQ_LO)
    assert (hi << 16 | lo) == FREQ, f"freq=0x{hi:04x}{lo:04x}, expected 0x{FREQ:08x}"

    # datapath: the DDS should now be producing a varying sample
    live = [to_signed(await fmc_read(dut, SAMPLE_ADDR)) for _ in range(6)]
    assert any(s != 0 for s in live), f"expected audio, got {live}"
    assert len(set(live)) > 1, f"sample should vary (saw), got {live}"

    # clear the gate -> silence
    await fmc_write(dut, VOICE0_CTRL, voice_ctrl(gate=0, wave=1, level=0xFF))
    await ClockCycles(dut.clk, 60)
    silent = [to_signed(await fmc_read(dut, SAMPLE_ADDR)) for _ in range(4)]
    assert all(s == 0 for s in silent), f"gate off should be silent, got {silent}"

    dut._log.info(f"ACM datapath OK: live={live}, silent={silent}")
