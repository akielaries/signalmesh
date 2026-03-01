#!/usr/bin/env python3

import RPi.GPIO as GPIO
import time

# Use GPIO pin 17 (change to whichever pin you connected)
CONTROL_PIN = 26

# Setup
GPIO.setmode(GPIO.BCM)
GPIO.setup(CONTROL_PIN, GPIO.OUT)
GPIO.output(CONTROL_PIN, GPIO.LOW)  # Start with button released

print("Toggling FPGA power...")
print("Holding 'button' for 2 seconds.")

# Press the button (turn transistor on)
GPIO.output(CONTROL_PIN, GPIO.HIGH)

# Wait 2 seconds
time.sleep(3)

# Release the button
GPIO.output(CONTROL_PIN, GPIO.LOW)

print("Done.")
GPIO.cleanup()
