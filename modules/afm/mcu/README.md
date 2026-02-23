# Audio FX Module
The idea here is:
- To have a bunch of parallel processing in RTL on the FPGA for filters and FX
- Audio data delivery over some physical medium. ATM I'm thinking I2S
- Cortex M1 MCU  handling control via AHB registers


This project uses a Gowin GW5AST FPGA on the Tang Mega 60k and the ARM Cortex M1
softcore IP provided by Gowin. Note that any of the Gowin supported FPGAs can be
used. I used the Gowin IDE to configure all settings

# Requirements
- A Gowin supported FPGA with the ARM Cortex M1 softcore configured with:
    - UART1
    - GPIO
    - Debug (Serial Wire)
- `arm-none-eabi-gcc` toolchain
- `gdb-multiarch`
- CMake

# How to build
From the root directory:
```
cmake -S . -B build/
cmake --build build/
```
and that will produce the compile ELF under build/bin/softcore_fw_example.elf.

To load the firmware:
```
gdb-multiarch build/bin/softcore_fw_example.elf

# connect to your debugger
(gdb) target extended-remote /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v2.0.0-323-gf3b99649-dirty_8A8243AE-if00
Remote debugging using /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v2.0.0-323-gf3b99649-dirty_8A8243AE-if00

# scan for available SWD targets
(gdb) mon swd
Target voltage: 0.48V
Available Targets:
No. Att Driver
 1      Generic Cortex-M1 M1

# attach, load, start
(gdb) att 1
Attaching to program: /home/akiel/GMD_workspace/softcore_fw_example/build/bin/softcore_fw_example.elf, Remote target
⚠️ warning: while parsing target memory map (at line 1): Required element <memory> is missing
delay_ms (nms=50) at /home/akiel/GMD_workspace/softcore_fw_example/src/delay.c:89
89			while((temp & 0x01) && !(temp & (1 << 16)));
(gdb) load
Loading section .text, size 0x1cb4 lma 0x0
Loading section .ARM.extab, size 0x3c lma 0x1cb4
Loading section .ARM.exidx, size 0xd8 lma 0x1cf0
Loading section .data, size 0x10 lma 0x1dc8
Start address 0x000001cc, load size 7640
Transfer rate: 84 KB/sec, 694 bytes/write.
(gdb) c
Continuing.
```
