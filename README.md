- [signalmesh](#signalmesh)
- [Technical bits and features](#technical-bits-and-features)
  * [The hardware](#the-hardware)
    + [The Audio Creation Module (ACM)](#the-audio-creation-module--acm-)
    + [The Data Siphon Module (DSM)](#the-data-siphon-module--dsm-)
      - [Siphon](#siphon)
      - [Preset Storage](#preset-storage)
    + [The Telemetry & Debug Module (TDM)](#the-telemetry---debug-module--tdm-)
    + [The Bootloader and Update Module (BUM!!)](#the-bootloader-and-update-module--bum---)
  * [The infrastructure](#the-infrastructure)
    + [`mkupdater`](#-mkupdater-)
  * [Planned protocols](#planned-protocols)
  * [Planned drivers](#planned-drivers)

# signalmesh
**signalmesh** aims to be a digital synthesizer and open source
hardware/software project

# Technical bits and features
Think of this as a light requirements spec/idea list

## The hardware
### The Audio Creation Module (ACM)
* USB 3.0 MIDI input to STM32H755
* STM32H755 processes MIDI packets and sends them to a TangNano 9k FPGA
* TangNano 9k FPGA acts as a synthesizer/oscillation engine
* TangNano 9k OR TangNano 1k FPGA act as an FX engine
* Output from FPGA to STM32H755 via FMC (Flexible Memory Controller)
implemented as an FPGA block/component/module. The FPGA delivers this
in a parallel fashion over several GPIO pins on the board to the
dedicated FMC pins (idk if these exist, needs research)
* STM32H755 delivers audio data over its DAC do internal speakers
or to external ones if connected

### The Data Siphon Module (DSM)
* This module has 2 purposes. Audio data siphoning + recording and
synth preset storage
#### Siphon
* STM32F33 will listen/poll/act upon an interrupt when audio data is
delivered to a set of pins, siphoning the data from the STM32H755's
DAC. If and only if a media device is connected, otherwise this is
wasted cycles
* The siphoned data will be delivered via USB to a connected media
device (SSD, thumbdrive, etc)

#### Preset Storage
Still needs more though on how all settings/parameters will be
inserted in the first place
* Record presets and save them to redundant pieces of I2C flash. Some
formatting will be needed here in the flash
* Probably some other stuff too

### The Telemetry & Debug Module (TDM)
* A STM32F103C8 will act as a hub for delivering all of the systems
debug messages over UART. This can get fancy and sophisticated, how can
it be made simple?
* Interfaces to various sensors for power consumption, climate data,
thermistors, some other valuable stuff idk
* Takes in that data and others like counters from various devices as
telemetry (which culd be also reported as debug messages). Ideally this
can be delivered from each board via CAN to this common module

### The Bootloader and Update Module (BUM!!)
* Another STM32F103C8 that acts as a sort of simple brain for the circuit
that is responsible for orchestrating updates to other devices and itself
* This device is able to read and write to redundant flash parts
* Able to flip between firmware images and bitsreams from flash, orchesrating
what the devices down the chain should boot
* There will likely be a line like SPI to the devices down the chain but
some logic about protecting what device is written to

## The infrastructure
### `mkupdater`
`mkupdater` will be a C++ command line application that takes in a JSON
manifest file like so:

```json
{
  "acm_fpga": {
    "HWID": "ACM",
    "name": "Audio Creation Module",
    "version": "2.0.0",
    "filepath": "build/acm/acm_fpga.bin"
  },
  "acm_mcu": {
    "HWID": "ACM",
    "name": "Audio Creation Module",
    "version": "0.11.0",
    "filepath": "build/acm/acm_mcu.elf"
  },
  "dsm_mcu": {
    "HWID": "DSM",
    "name": "Data Siphon Module",
    "version": "1.2.0",
    "filepath": "build/dsm/dsm_mcu.elf"
  },
  "tdm_mcu": {
    "HWID": "TDM",
    "name": "Telemetry and Debug Module",
    "version": "0.23.1",
    "filepath": "build/tdm/tdm_mcu.elf"
  },
  "bum_mcu": {
    "HWID": "BUM",
    "name": "Bootloader and Update Module",
    "version": "0.3.0",
    "filepath": "build/bum/bum_mcu.elf"
  }
}
```

From these manifest contents we should be able to create a single
binary file. This binary file will containing the code to flash
to each device in the manifest (ACM, DSM, etc). The BUM will be
responsible for parsing this blob of data and breaking it up to
be sent to each device (not sure the protocol yet, SPI is our
fastest option I think)

## Planned protocols
* UART
* SPI
* I2C
* USB
* FMC
* CAN
Each of these will need planning around what their packets look
like

## Planned drivers
Here is a list of planned drivers, interfaces, and just general
backbone code we'll need for *meshing* all of the hardware together:


