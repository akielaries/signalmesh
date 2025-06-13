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
* STM32H755
* STM32H753
    * ARM M7 and M4, idk how to use the M4 right now
* STM32F33
    * ARM M4. Specs say this is optimal for mixed signal work
* STM32F103
    * Cheap POS bluepill as the dev board
* GW1NR-9
    * Cheap POS TangNano9k as the dev board

### The Audio Creation Module (ACM)
* USB 3.0 MIDI input to STM32H755
* STM32H755 processes MIDI packets and sends them to a TangNano 9k FPGA
* TangNano 9k FPGA acts as a synthesizer/oscillation engine
* TangNano 9k OR TangNano 1k FPGA act as an FX engine
* Output from FPGA to STM32H755 via FMC (Flexible Memory Controller)
implemented as an FPGA block/component/module. The FPGA delivers this
in a parallel fashion over several GPIO pins on the board to the
dedicated FMC pins (idk if these exist, needs research)
* I think having the FPGA deliver parallel stereo data over two GPIOs
to the H755 may be the route to take here
* STM32H755 delivers stereo audio data over its DAC pins to internal speakers
or to external ones if connected

### The Data Siphon Module (DSM)
* This module has 2 purposes. Audio data siphoning + recording and
synth preset storage
#### Siphon
* STM32H753 will listen/poll/act upon an interrupt when audio data is
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
* A STM32F33 will act as a hub for delivering all of the systems
debug messages over UART. This can get fancy and sophisticated, how can
it be made simple?
* Interfaces to various sensors for power consumption, climate data,
thermistors, some other valuable stuff idk
* Takes in that data and others like counters from various devices as
telemetry (which culd be also reported as debug messages). Ideally this
can be delivered from each board via CAN to this common module
* Failure state and telemetry should be stored in a flash device

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
* UART (Universal Asynchronous Receiver-Transmitter)
* SPI (Serial Peripheral Interface)
* I2C (Inter-Intergrated circuit)
* USB (Universal Serial Bus, 2.0 and 3.0?)
* CAN (Controller Area Network)
* MIDI (Musical Instrument Digital Interface)
* FMC (Flexible Memory Controller)

Each of these will need planning around what their packets look
like

## Planned drivers
Here is a list of planned drivers needed for hardware

* 24LC256 32-kb flash (I2C)
    * `24lc256.c`
* additional RAM/memory drivers as needed
    * `<partnumber>.c`
* INA219 26V single channel DC current sensor (I2C)
    * `ina219.c`
* INA3221 26V 3-channel current sensor (I2C)
    * `ina3221.c`
* 16x02, 20x04, and or bigger displays (I2C, perhaps SPI?)
    * `1602lcd.c`, `2004lcd.c`
* LN513RA 7 segment display for counters
    * `ln513ra.c`
* BME280 temperature, humidity, pressure sensor
    * `bme280.c`
* W25Q64 QSPI flash
    * `w25q64.c`

* Network of thermistors across the board
    * `thermistor.c`


## Planned Interfaces
Backbone interfaces and code we'll need for *meshing* all of the
hardware together:

* `flash.c` - I2C and SPI flash interface. Should the FMC interface
sit in here too?
* `telemetry.c` - Telemetry interface

# Incremental steps!
* Wire up in breadboards and perfboards what I have in mind with all of the
different MCUs and FPGAs
* Create a KiCAD schematic of these connections. The PCB will come later
* Create some common code with the help of Chibi. Each MCU will
need some common code like:
    * Debug port prints via UART
    * Bootup sequences. Flash onboard+external LEDs on startup
    * and some other common functions I imagine

## UART
I sort of have this one figured out. I can send UART packets to my raspberry
pi and decode them no problem with an oscilloscope as well. This is part of
the debug print step mentioned aboved

```
sm_printf.c
```

## I2C
There are several I2C devices in the mix. Get started with a driver for each:

```
bme280.c
lcd1602.c
lcd2004.c // may not need an additional driver?
24lc256.c
ina219.c
ina3221.c
```

Most of these devices are specific to the TDM (Telemetry & Debug Module)
but the DSM (Data Siphon Module) will have a preset display and write to
a 24LC256 for the preset table


## USB
I have a micro USB pinout interface but no clue how it operates. Appears
to be a 2-pin board. I need to get an idea of what USB packets look like
and how the MIDI packets are encapsulated (I assume that's how it works).
That aside, I'm aspiring to have the following USB devices:
* Debug port - OUTPUT
* Media (maybe SD is better for this...) port - OUTPUT
* Update port - INPUT
* Maybe more?

I'm unsure of how many interfaces this will produce but I imagine it
would call the same underlying USB interface/driver...

Just getting debug print messages from 1 then all boards to a single
USB port would be sweet. Can I leverage an FTDI for this??

## SPI
I plan to store the firmware and bitstream updates in redundant SPI
flash parts. The redundancy should probably come a background task
or maybe some copy on write stuff I'm unfamiliar with. It seems like
the STM32F103 I want orchestrating all of this has two SPI interfaces
and I hope this is the case so one of them can read from flash and the
other can propogate that data to the slaves. I believe the STM32F103
in this case acts as the single *master* and the rest of the devices
are made *slaves* via the `CS` chip select pin

I think the output of this is going to be some bringup SPI code and
perhaps a driver for the 8mb W25Q64 QSPI flash. I'm really curious how
I can utilize this

```
w25q64.c
```

## CAN
I'm unfamiliar with CAN packets/frames. Will need to dig into how these
look and how I can daisy chain my TD/RD lines while decoding the data
into something sensible on the TDM (STM32F33)

Output of this is likely the beginnings of the telemtry implementation

## Interrupts
How does the STM32H7 handle interrupts? How do we implement this? There's
tons of value to these in our case and 1 of them is on update. There will
be a designated pin for signaling an update wants to take place and that
should probably have something to do with interrupts!?

## Digital Audio Conversion
The H755 should be delivering the file audio data while the H753 sniffs it
and writes that data to media (if we're recording). How can I get started
with simply generating sounds from the H755 -> some speakers

...

# Some nice to haves
* Servo motors that reset the potentiometers back to "0" on the hit
hit of a button/switch. I think this would be cool
    * Idea here could be a "third party" wire listenting in on the
    voltage output from the potentiometer and as it is listenting
    adjust it back to 0 with the servo
