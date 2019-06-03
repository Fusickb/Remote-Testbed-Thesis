# Overview

Each used file will be summarized here to give a brief overview of the functionality of the Tornado server https://www.tornadoweb.org/en/stable/

app.py

- Creates regular expressions for URL and ties it to a class defined withing NSFviews.py to function as the request handler
- File to be executed, main application

canTools.py
- Transmit messages onto the vehicle network
- Uses cangen to loop messages on vehicle network
- Perform parameter updates for VIN and Governor Speed

ExperimentHandler.py
- Schedule commands including simulation commands, parameter updates, transmitting CAN frames, and cangen

SerialFunctions.py
- Performs serial commands for simulation commands used within an experiment (setting axle based vehicle speed, toggling ignition, etc.)
- Performs serial commands for "Live Data"

NSFviews.py
- Request handler for commands to start an experiment, set speed for "Live Data" using SerialFunctions.py, perform vehicle network-based attacks for "Live Data", and adjust pin settings for "Live Data"

NSFsettings.py
- Settings for Tornado server