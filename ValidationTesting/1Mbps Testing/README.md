# Overview

Source code for validation testing documented in Chapter 4 split into 3 directories

## Arduino ino file and FlexCAN Library

CANTeensyTest.ino

- Firmware used on the Smart Sensor Simulator 2 (SSS2) as the transmitter and receiver for the 1Mbps 100% busload frame loss testing
- The boolean isTx causes the SSS2 to act as a transmitter if true and a receiver if false
- Transmitter has two operational modes: transmitting and printing the number of trasmissions over serial, toggled by the click of the button
- The transmitter/receiver SSS2 pair (on two seperate SSS2's) was used to validate the Teensy 3.6 is capable of sending 100% busload at 1Mbps
- The transmitter can be used alone to validate different types of equipment

The test frames are in the following format.

    ------------------------------------------
    | CAN ID | TX Counter | Timestamp Millis |
    ------------------------------------------

* CAN ID: A fixed extended ID of 00111111.
* CAN Data (TX Counter and Timestamp Millis).
	- TX Counter: Four bytes to indicate the number of the transmitted frame, increments after each transmission.
	- Timestamp Millis: Four bytes to store the timestamp in milliseconds.


NOTE: Install FlexCAN and replace the directory named "src" where FlexCAN is installed with "FlexCAN src" in this directory.

## canCounter.py

- Used with the CANTeensyTest.ino transmitter for BeagleBone with TruckCape testing
- Simple script to increment a counter when receiving a CAN frame and print the value of the counter afterwards

## cpuUtil.py

- Uses the Linux top command and sums the CPU usage of each process from 1 iteration of top (looped)
- Continuously prints the total CPU usage until "CTRL+C" is pressed