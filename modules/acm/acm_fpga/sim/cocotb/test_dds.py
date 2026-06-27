"""cocotb testbench for dds_synth (polyphonic DDS oscillator bank).

drives the flattened per-voice control buses, pulses the sample tick, and checks:
  - silence when no voice is gated
  - a gated saw ramps the output
  - gate=0 and level=0 both mute a voice
  - two identical voices sum to ~2x one voice

run:  make TARGET=dds
"""

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import ClockCycles

NVOICES = 8

# python-side mirror of the voice control; apply() packs it into the flat buses
voices = [dict(freq=0, gate=0, wave=0, level=0) for _ in range(NVOICES)]


def to_signed(v, bits=16):
    v &= (1 << bits) - 1
    return v - (1 << bits) if (v & (1 << (bits - 1))) else v


def apply(dut):
    freq_flat = gate_flat = wave_flat = level_flat = 0
    for i, v in enumerate(voices):
        freq_flat  |= (v["freq"]  & 0xFFFFFFFF) << (i * 32)
        gate_flat  |= (v["gate"]  & 0x1)        << i
        wave_flat  |= (v["wave"]  & 0x7)        << (i * 3)
        level_flat |= (v["level"] & 0xFF)       << (i * 8)
    dut.freq_flat.value  = freq_flat
    dut.gate_flat.value  = gate_flat
    dut.wave_flat.value  = wave_flat
    dut.level_flat.value = level_flat


async def do_tick(dut):
    dut.tick.value = 1
    await ClockCycles(dut.clk, 1)
    dut.tick.value = 0
    for _ in range(NVOICES + 6):
        await ClockCycles(dut.clk, 1)
        if int(dut.sample_valid.value) == 1:
            break
    return to_signed(int(dut.sample_o.value), 16)


async def reset(dut):
    for v in voices:
        v.update(freq=0, gate=0, wave=0, level=0)
    apply(dut)
    dut.tick.value = 0
    dut.rst.value = 1
    await ClockCycles(dut.clk, 5)
    dut.rst.value = 0
    await ClockCycles(dut.clk, 5)


@cocotb.test()
async def dds_test(dut):
    cocotb.start_soon(Clock(dut.clk, 20, unit="ns").start())
    await reset(dut)

    # 1) silence - nothing gated
    s = await do_tick(dut)
    assert s == 0, f"silence: expected 0, got {s}"

    # 2) gated saw on voice 0 should ramp the output upward
    voices[0].update(freq=0x0200_0000, gate=1, wave=1, level=255)
    apply(dut)
    prev, rising = None, 0
    for _ in range(6):
        s = await do_tick(dut)
        if prev is not None and s > prev:
            rising += 1
        prev = s
    assert rising >= 4, f"saw should ramp up, only {rising}/5 rising steps"

    # 3) gate off mutes it
    voices[0]["gate"] = 0
    apply(dut)
    assert (s := await do_tick(dut)) == 0, f"gate off: expected 0, got {s}"

    # 4) gate on but level 0 also mutes
    voices[0].update(gate=1, level=0)
    apply(dut)
    assert (s := await do_tick(dut)) == 0, f"level 0: expected 0, got {s}"

    # 5) two identical voices sum to ~2x one voice (both start at phase 0 post-reset)
    await reset(dut)
    voices[0].update(freq=0x0080_0000, gate=1, wave=1, level=255)
    apply(dut)
    one = await do_tick(dut)
    one = await do_tick(dut)            # a couple ticks in, away from 0

    await reset(dut)
    voices[0].update(freq=0x0080_0000, gate=1, wave=1, level=255)
    voices[1].update(freq=0x0080_0000, gate=1, wave=1, level=255)
    apply(dut)
    two = await do_tick(dut)
    two = await do_tick(dut)
    # same phase/freq -> two voices ~= 2x one (allow rounding slack)
    assert abs(two - 2 * one) <= 4, f"2 voices ({two}) != ~2x one ({one})"

    dut._log.info(f"DDS checks passed (one={one}, two={two})")
