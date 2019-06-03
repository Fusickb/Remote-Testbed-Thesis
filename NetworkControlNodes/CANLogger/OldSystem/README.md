# Overview

Each used file will be summarized here to give a brief overview of the functionality of the Tornado server https://www.tornadoweb.org/en/stable/

app.py

- Creates regular expressions for URL and ties it to a class defined withing NSFviews.py to function as the request handler
- File to be executed, main application

canTools.py
- Read messages from the vehicle network

NSFlogger.py
- Starts threads for each CAN interface to read and write to a file for an experiment
- Starts threads for each CAN interface to read and transfer the message to the front interface for "Live Data"

NSFutilities.py
- Obtain the next JSON object to transmit when transferring a log file from the CAN Logger to the front interface
- Returns plottable format of data to transmit to front interface for plotting an experiment

NSFviews.py
- Request handler for commands to get the transfer the log file to the front interface, get plottable data (speed or updatable parameter), start logging, and start streaming messages "Live Data"

NSFsettings.py
- Settings for Tornado server