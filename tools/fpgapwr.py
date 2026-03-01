#!/usr/bin/env python3

import RPi.GPIO as GPIO
import time

CONTROL_PIN = 26

GPIO.setmode(GPIO.BCM)
GPIO.setup(CONTROL_PIN, GPIO.OUT)
# start w button "released"
GPIO.output(CONTROL_PIN, GPIO.LOW)

print("Toggling FPGA power...")
print("Holding for 3 seconds...")

# press the button (turn transistor on)
GPIO.output(CONTROL_PIN, GPIO.HIGH)

time.sleep(3)

# release the button
GPIO.output(CONTROL_PIN, GPIO.LOW)

print("Done....")
GPIO.cleanup()
