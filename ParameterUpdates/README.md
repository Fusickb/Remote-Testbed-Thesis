# Overview

Source code for parameter updates case study documented in Chapter 5

# Shell Scripts

Before reading or writing parameters, execute both diagMessages.sh and flowControl.sh shell scripts (used on BeagleBone with custom Linux distribution) and perform the following steps in a terminal

1) Enter `sudo chmod +x diagMessages.sh` to make diagMessages.sh executable

2) Enter `sudo chmod +x flowControl.sh` to make flowControl.sh executable

3) Execute both scripts: `./diagMessages` and `./flowControl` in two seperate terminals

## Reading Parameters

1) With the shell scripts running, open a new terminal and execute the Python script readVin.py by `python3 readVin.py` to read VIN

2) In the new terminal, execute the Python script readGovSpeed.py by `python3 readGovSpeed.py` to read the governor speed

## Updating Parameters

NOTE: This will not work unless you have the algorithm to your seed-key exchange. This has been modified for security purposes

The seed-key exchange for updating VIN and governor speed are identical aside from a register write command. Therefore, the seedKeyExchange.py is associated with vinRewrite.py and seedKeyExchangev2.py is associated with govRewritev2.py.

The appropriate seed-key exchange script should be executed before attempting either rewrite script.