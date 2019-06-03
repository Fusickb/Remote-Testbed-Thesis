# Overview

Source code for validation testing documented in Chapter 4 split into 3 directories

250kFrameDrop.ino

- Firmware used on SSS2 as the transmitter for the 250kbps 100% busload frame loss testing

The test frames are in the following format.

    ------------------------------------------
    | CAN ID | TX Counter | Timestamp Millis |
    ------------------------------------------

* CAN ID: A fixed extended ID of 00111111.
* CAN Data (TX Counter and Timestamp Millis).
	- TX Counter: Four bytes to indicate the number of the transmitted frame, increments after each transmission.
	- Timestamp Millis: Four bytes to store the timestamp in milliseconds.


NOTE: Install FlexCAN and replace the directory named "src" where FlexCAN is installed with "FlexCAN src" in this directory.