#!/usr/bin/python3
import subprocess
import os
import signal
import argparse
import threading
from datetime import datetime

# device : {serial, port}
device_list = {
        'STM32F334' : {
            "serial":"0668FF393355373043073856",
            "port":3344
            },
        'STM32F446' : {
            "serial":"0668FF363154413043233728",
            "port":4466
            },
        'STM32H755' : {
            "serial":"0030004A3532510F31333430",
            "port":7555
            },
        'STM32H753' : {
            "serial":"003500323532511331333430",
            "port":7533
            },
        'STM32F103' : {
            "serial":"18006000010000543931574E",
            "port":1033
            },
        }


def stream_output(process, name):
    for line in process.stdout:
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}][{name}] {line.strip()}")

def print_devices():
    for device in device_list:
        print(f"device: {device}")
        print(f"    serial: {device_list[device]['serial']}")
        print(f"    port  : {device_list[device]['port']}")
    print("")

def kill_gdbserver():
    print("Killing GDB servers")
    try:
        output = subprocess.check_output(["pgrep", "-f", "st-util"])
        pids = output.decode().splitlines()
        for pid in pids:
            print(f"Killing existing st-util process: PID {pid}")
            os.kill(int(pid), signal.SIGTERM)
    except subprocess.CalledProcessError:
        # No st-util processes running
        pass

import shlex

def start_gdbserver(monitor=False):
    print("Starting GDB server for devices:")
    print_devices()

    for device, info in device_list.items():
        serial = info['serial']
        port = info['port']
        cmd = f"st-util --serial {serial} -p {port}"
        print(f"Launching st-util for {device} on port {port}")

        if monitor:
            process = subprocess.Popen(shlex.split(cmd),
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT,
                                       text=True)
            threading.Thread(target=stream_output, args=(process, device), daemon=True).start()
        else:
            subprocess.Popen(shlex.split(cmd),
                             stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL)

def list_gdbservers():
    serial_to_name = {info["serial"]: name for name, info in device_list.items()}

    try:
        output = subprocess.check_output(["pgrep", "-af", "st-util"], text=True)
        lines = output.strip().splitlines()

        if not lines:
            print("No st-util processes running.")
            return

        print("Active GDB (st-util) sessions:")
        for line in lines:
            parts = line.split()
            pid = parts[0]
            cmd = " ".join(parts[1:])

            # Extract serial number
            serial = None
            for i, token in enumerate(parts):
                if token == "--serial" and i + 1 < len(parts):
                    serial = parts[i + 1]
                    break

            device_name = serial_to_name.get(serial, "Unknown")
            print(f"  {pid} {cmd}   ({device_name})")

    except subprocess.CalledProcessError:
        print("No st-util processes running")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-s",
        "--start",
        action="store_true",
        help="Start GDB servers for the defined devices"
    )
    parser.add_argument(
        "-m", "--monitor",
        action="store_true",
        help="Monitor and print st-util output to terminal"
    )
    parser.add_argument(
        "-k",
        "--kill",
        action="store_true",
        help="Kill all GDB servers"
    )
    parser.add_argument(
        "-l",
        "--list",
        action="store_true",
        help="List running GDB servers"
    )
    args = parser.parse_args()

    if args.start:
        start_gdbserver(args.monitor)
    elif args.kill:
        kill_gdbserver()
    elif args.list:
        list_gdbservers()
    else:
        print("Missing args. --start/-s or --kill/-k")


if __name__ == '__main__':
    main()
