- [signalmesh](#signalmesh)
- [Technical bits and features](#technical-bits-and-features)
  * [The hardware](#the-hardware)
  * [The Audio Acquisition Module (AAM)](#the-audio-acquisition-module--aam-)
  * [The Audio Creation Module (ACM)](#the-audio-creation-module--acm-)
  * [The Data Siphon Module (DSM)](#the-data-siphon-module--dsm-)
    + [Siphon](#siphon)
    + [Preset Storage](#preset-storage)
  * [The Telemetry & Debug Module (TDM)](#the-telemetry---debug-module--tdm-)
  * [The Upload and Update Module (UUM)](#the-upload-and-update-module--uum---)
  * [The infrastructure](#the-infrastructure)
    + [`mkupdater`](#-mkupdater-)
  * [Planned protocols](#planned-protocols)
  * [Planned drivers](#planned-drivers)
  * [Planned Interfaces](#planned-interfaces)
- [Incremental steps!](#incremental-steps-)
  * [UART](#uart)
  * [I2C](#i2c)
  * [USB](#usb)
  * [SPI](#spi)
  * [CAN](#can)
  * [Interrupts](#interrupts)
  * [Digital Audio Conversion](#digital-audio-conversion)
- [Some nice to haves](#some-nice-to-haves)

# signalmesh
**signalmesh** aims to be a digital synthesizer and open source
hardware/software project

# Details
## Modules
There are 5 boards active in the **signalmesh** circuit. Three of which are concerned
with sound (ACM, AAM, DSM) and the remaining two (UUM, TDM) are related to health,
updating, and other housekeeping related items.

### Audio Creation Module (ACM)
GWIN-9R Tangnano9k FPGA
* Responsible for sound generation


### Audio Acquisition Module (AAM)
STM32H755

* Takes potentiometers, switches, etc. for sound parameters.
* Sends audio creation parameters to the ACM via UART.
* Receives the audio data from the FPGA via FMC.
* Outputs the audio data over the DAC.


### Data Siphon Module (DSM)
STM32H753

* If external media is present and recording is enabled, the played data will be
siphoned from the AAM and to the external media.

### Update and Upload Module (UUM)
STM32F446

* Able to update and switch firmware versions of all on board devices.
* Firmware packages can be made via `mkupdater.

### Telemetry and Debug Module (TDM)
STM32F334

* Collects telemetry throughout the system via CAN and collects debug messages
via UART. Ready to display via two USB 2.0 ports respectively.
* Displays useful health/system information to one of the on board LCD screens.
