# Overview

Source code for the Smart Sensor Simulator 2's http://www.synercontechnologies.com/sss2/ of the Remote Access Testbed. https://github.com/jeremy-daily/SSS2

This README will briefly explain the three files. Note that if library issues occur, refer to either a FlexCAN library within ValidationTesting of this repository or the linked SSS2 GitHub repository.

SSS2_Firmware.ino

- Main file/image for the Smart Sensor Simulator 2

SSS2_board_defs_rev_2.h

- Contains board definitions, pin settings, default variables, and pin values for setup

SSS2_functions.h

- Contains functions including adjusting voltage, speed change event, setting frequency, etc.