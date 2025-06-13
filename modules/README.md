# Modules
These are the main board components of the `signalmesh` project

## Audio Acquisition Module (AAM)
STM32H755

Take USB MIDI input and send those packets to the GW1NR-9 FPGA via UART
to process

Receives stereo data from the FPGA via the FMC (Flexible Memory Controller)

Stereo output data via the DAC pins or perhaps I2S (worth looking into)

## Audio Creation Module (ACM)
GW1NR-9

Engines for UART processing, MIDI processing, oscillation, FX, etc and generating
stereo output at a target of 48kHz (is this adjustable easily?)

Sends parallel stereo data back to the AAM via the FMC

## Data Siphon Module (DSM)
STM32H753

Manages presets in I2C flash

Siphons audio data and writes it to media if connected

Display preset data to I2C 2004 LCD

## Telemetry Debug Module (TDM)
ST32F33
Has several I2C devices:
* BME280
* LCD1602
* LCD2004
* 24LC256
* INA219
* INA3221

A network of thermistors laid throughout the circuit

Master CAN node for telemetry streaming from other devices

Stores a crash table AKA telemetry snapshot in an I2C flash

## Bootloader and Updater Module (BUM)
STM32F103

This device might need an upgrade. The goal is simple, a stripped down bootloader
that is able to select images to boot for each device in the circuit. Meaning it
is capable of updates

There should ideally be a USB female hookup to this device

We will read/write data to/from a 8mb W25Q64 QSPI flash
