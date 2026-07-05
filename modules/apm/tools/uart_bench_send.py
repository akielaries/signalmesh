#!/usr/bin/env python3
# host side of the USART3 RX benchmark: blast N raw bytes at the STM32 and time
# it. computes the same fnv-1a the firmware does so you can confirm every byte
# survived. the firmware ends a transfer on a ~250ms idle gap, so just send and
# stop.
#
# usage:
#   ./uart_bench_send.py /dev/ttyUSB0 --baud 2000000 --size 4M
#   ./uart_bench_send.py /dev/ttyUSB0 --baud 2000000 --size 577178 --seed 1

import argparse
import os
import random
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial not found: pip install pyserial")


def parse_size(s):
    # accept plain bytes or K/M suffixes
    s = s.strip().upper()
    mult = 1
    if s.endswith("K"):
        mult, s = 1024, s[:-1]
    elif s.endswith("M"):
        mult, s = 1024 * 1024, s[:-1]
    return int(float(s) * mult)


def fnv1a(data):
    h = 0x811C9DC5
    for b in data:
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h


def main():
    ap = argparse.ArgumentParser(description="UART RX benchmark sender")
    ap.add_argument("port")
    ap.add_argument("--baud", type=int, default=2000000)
    ap.add_argument("--size", type=parse_size, default="1M")
    ap.add_argument("--seed", type=int, default=None,
                    help="reproducible payload (else os.urandom)")
    args = ap.parse_args()

    if args.seed is not None:
        data = random.Random(args.seed).randbytes(args.size)
    else:
        data = os.urandom(args.size)

    checksum = fnv1a(data)
    print(f"port {args.port}  baud {args.baud}  size {len(data)} bytes")
    print(f"payload fnv1a: 0x{checksum:08X}")

    # raw, no flow control - must match firmware (cr3=0)
    ser = serial.Serial(args.port, args.baud, timeout=1,
                        rtscts=False, xonxoff=False, dsrdtr=False)

    t0 = time.perf_counter()
    ser.write(data)
    ser.flush()  # block until the OS has handed off all bytes
    t1 = time.perf_counter()
    ser.close()

    dt = t1 - t0
    if dt <= 0:
        dt = 1e-6
    kbps = (len(data) / 1024.0) / dt
    mbits = (len(data) * 8.0) / (dt * 1e6)
    line_bps = args.baud / 10.0
    util = (len(data) / dt) / line_bps * 100.0

    print(f"sent in  : {dt * 1000.0:.1f} ms")
    print(f"host rate: {kbps:.1f} KB/s  ({mbits:.2f} Mbit/s)")
    print(f"line util: {util:.1f} %")
    print("compare payload fnv1a above with the firmware's console output")


if __name__ == "__main__":
    main()
