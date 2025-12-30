# signalmesh
*signalmesh* is a hybrid analog/digital synthesizer. For now it is a mess of reworked PCBs, perf/breadboards, and wires on my desk.

CAD work going on here right now: https://github.com/akielaries/signalmesh_CAD/

# Details
Don't forget to initialize submodules. ChibiOS is the RTOS used in this project

```
git submodule init
git submodule update
```

## Modules
I anticipated having a few MCUs in the circuit but am starting off small for now with
a single STM32H755 that will be doing all of the lifting. I'm doing all the development
on this under `modules/apm`.

# Development
Debug port access is simple and there are several examples throughout the code. Use
any USB -> Serial TTY converter and hook up the TX/RX pins to UART5 (`SD5`) on
the STM32. I use `tio` to view the debug port activity at 1,000,000 baud.

For development there is `tools/gdbserver.py` which is helpful for starting,
killing, listing, and monitoring GDB server sessions for different board flavors.
It is worth noting the serial numbers/ST-link IDs are specific to my setup.
```
$ ./tools/gdbserver.py --help
usage: gdbserver.py [-h] [-s] [-m] [-k] [-l]

options:
  -h, --help     show this help message and exit
  -s, --start    Start GDB servers for the defined devices
  -m, --monitor  Monitor and print st-util output to terminal
  -k, --kill     Kill all GDB servers
  -l, --list     List running GDB servers
```
When invoking the script notice I upload programs to two of the listed devices
(H755, H753):
```
$ ./tools/gdbserver.py --start --monitor                                                                     [5:20:09]
Starting GDB server for devices:
Launching st-util for STM32F334 on port 3344
Launching st-util for STM32F446 on port 4466
Launching st-util for STM32H755 on port 7555
[05:20:11][STM32F334] use serial 0668FF393355373043073856
[05:20:11][STM32F334] st-util
Launching st-util for STM32H753 on port 7533
[05:20:11][STM32F446] use serial 0668FF363154413043233728
[05:20:11][STM32F446] st-util
[05:20:11][STM32H755] use serial 0030004A3532510F31333430
[05:20:11][STM32H755] st-util
[05:20:11][STM32H753] use serial 003500323532511331333430
Launching st-util for STM32F103 on port 1033
[05:20:11][STM32H753] st-util
[05:20:11][STM32F103] use serial 18006000010000543931574E
[05:20:11][STM32F103] st-util
[05:20:11][STM32H755] 2025-06-17T05:20:11 INFO usb.c: Unable to match requested speed 1800 kHz, using 1000 kHz
[05:20:11][STM32H753] 2025-06-17T05:20:11 INFO usb.c: Unable to match requested speed 1800 kHz, using 1000 kHz
[05:20:11][STM32F446] 2025-06-17T05:20:11 INFO common.c: F446: 128 KiB SRAM, 512 KiB flash in at least 128 KiB pages.
[05:20:11][STM32F446] 2025-06-17T05:20:11 INFO gdb-server.c: Listening at *:4466...
[05:20:11][STM32H755] 2025-06-17T05:20:11 INFO common.c: H74x/H75x: 128 KiB SRAM, 2048 KiB flash in at least 128 KiB pages.
[05:20:11][STM32H755] 2025-06-17T05:20:11 INFO gdb-server.c: Listening at *:7555...
[05:20:11][STM32F334] 2025-06-17T05:20:11 INFO common.c: F334 medium density: 12 KiB SRAM, 64 KiB flash in at least 2 KiB pages.
[05:20:11][STM32F334] 2025-06-17T05:20:11 INFO gdb-server.c: Listening at *:3344...
[05:20:11][STM32H753] 2025-06-17T05:20:11 INFO common.c: H74x/H75x: 128 KiB SRAM, 2048 KiB flash in at least 128 KiB pages.
[05:20:11][STM32H753] 2025-06-17T05:20:11 INFO gdb-server.c: Listening at *:7533...
[05:20:11][STM32F103] 2025-06-17T05:20:11 WARN common.c: NRST is not connected
[05:20:11][STM32F103] 2025-06-17T05:20:11 INFO common.c: F1xx Medium-density: 20 KiB SRAM, 128 KiB flash in at least 1 KiB pages.
[05:20:11][STM32F103] 2025-06-17T05:20:11 INFO gdb-server.c: Listening at *:1033...
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO common.c: H74x/H75x: 128 KiB SRAM, 2048 KiB flash in at least 128 KiB pages.
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c: Found 8 hw breakpoint registers
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c: Chip clidr: 09000003, I-Cache: on, D-Cache: on
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c:  cache: LoUU: 1, LoC: 1, LoUIS: 0
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c:  cache: ctr: 8303c003, DminLine: 32 bytes, IminLine: 32 bytes
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c: D-Cache L0: 2025-06-17T05:20:24 INFO gdb-server.c: f00fe019 LineSize: 8, ways: 4, sets: 128 (width: 12)
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c: I-Cache L0: 2025-06-17T05:20:24 INFO gdb-server.c: f01fe009 LineSize: 8, ways: 2, sets: 256 (width: 13)
[05:20:24][STM32H755] 2025-06-17T05:20:24 INFO gdb-server.c: GDB connected.
[05:20:32][STM32H755] 2025-06-17T05:20:32 INFO common.c: H74x/H75x: 128 KiB SRAM, 2048 KiB flash in at least 128 KiB pages.
[05:20:32][STM32H755] 2025-06-17T05:20:32 INFO gdb-server.c: flash_erase: block 08000000 -> 20000
[05:20:32][STM32H755] 2025-06-17T05:20:32 INFO gdb-server.c: flash_erase: page 08000000
[05:20:33][STM32H755] 2025-06-17T05:20:33 INFO common.c: Starting Flash write for H7
[05:20:33][STM32H755] 2025-06-17T05:20:33 INFO gdb-server.c: flash_do: block 08000000 -> 20000
[05:20:33][STM32H755] 2025-06-17T05:20:33 INFO gdb-server.c: flash_do: page 08000000
[05:20:56][STM32H755] 2025-06-17T05:20:56 ERROR gdb-server.c: cannot recv: -2
[05:20:56][STM32H755] 2025-06-17T05:20:56 INFO gdb-server.c: Listening at *:7555...
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO common.c: H74x/H75x: 128 KiB SRAM, 2048 KiB flash in at least 128 KiB pages.
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c: Found 8 hw breakpoint registers
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c: Chip clidr: 09000003, I-Cache: on, D-Cache: on
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c:  cache: LoUU: 1, LoC: 1, LoUIS: 0
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c:  cache: ctr: 8303c003, DminLine: 32 bytes, IminLine: 32 bytes
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c: D-Cache L0: 2025-06-17T05:20:56 INFO gdb-server.c: f00fe019 LineSize: 8, ways: 4, sets: 128 (width: 12)
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c: I-Cache L0: 2025-06-17T05:20:56 INFO gdb-server.c: f01fe009 LineSize: 8, ways: 2, sets: 256 (width: 13)
[05:20:56][STM32H753] 2025-06-17T05:20:56 INFO gdb-server.c: GDB connected.
^C[05:21:29][STM32H753] Receive signal 2. Exiting...
```
And I get output from both


