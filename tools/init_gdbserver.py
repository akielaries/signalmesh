#!/usr/bin/python3
import subprocess
import os
import signal
import argparse


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


def print_devices():
    #command = "st-info --probe"
    #result = subprocess.check_output(command, shell=True, text=True)
    #print(result)

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

def start_gdbserver():
    print("Starting GDB server for devices:")
    print_devices()

    for device in device_list:
        serial = device_list[device]['serial']
        port = device_list[device]['port']

        cmd = f"st-util --serial {serial} -p {port}"
        print(cmd)
        subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-s",
        "--start",
        action="store_true",
        help="Start GDB servers for the defined devices"
    )
    parser.add_argument(
        "-k",
        "--kill",
        action="store_true",
        help="Kill all GDB servers"
    )
    args = parser.parse_args()

    if args.start:
        start_gdbserver()
    elif args.kill:
        kill_gdbserver()
    else:
        print("Missing args. --start/-s or --kill/-k")


if __name__ == '__main__':
    main()
