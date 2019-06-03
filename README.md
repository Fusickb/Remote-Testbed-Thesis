# Overview

This GitHub repository contains source code for the thesis "Remote Simulation of Heavy Vehicle Electronic and Communications Environment" by Blake Fusick and is split into 5 main directories. A brief summary of each directory will now be explained with more details inside each of the directories.

1) FrontInterface
	- Contains source code for the user interface portion of the testbed. Should be stored/executed on desired machine hosting the web server

2) NetworkControlNodes
	- Contains source code for each of the Network Control Nodes (BeagleBones with TruckCape)
	- Includes CAN Logger, Experiment Handler, and Simulator Controller
	- CAN Logger source code has both the original old logging system and the new logging system after validation testing

3) SensorSimulation
	- Contains source code used on Smart Sensor Simulator 2 for sensor and actuator simulation on this testbed

4) ValidationTesting
	- Contains source code used in validation testing
	- Includes the original 250k test FlexCAN library and ino file, the TCPCAN logging solution, and the 1Mbps testbed limit experiementation modified FlexCAN library and ino file
	- NOTE: The 250k test and 1Mbps use different FlexCAN modifications
	- NOTE: The TCPCAN logging solution is the same as the contents found at the GitHub link from number 3 in the useful links section of this README.md file

5) ParameterUpdates
	- Contains source code used for updating parameters
	- Includes the seed-key exchange (with default values for security purposes), reading parameters, writing parameter, and sending session messages

## Setting Up Testbed
Open the file FrontInterface/docs/build/html/index.html in a web browser to view the original documentation for the Front interface, primarily written by Ethan Robards. A set of installation and execution instructions are provided below. The old documentation is left unmodified and installation instructions found within may not be accurate.

Also note that the IP addresses used in the source code may not work since your network topology will likely be different. Be sure to adjust the IP addresses appropriately to allow the network nodes and front interface to communicate.

### Installation of the User Interface

To install required programs/libraries

1) Install Python3 and Git

2) Copy the contents of the FrontInterface directory into a desired directory on the desired machine

3) Navigate inside the directory where the FrontInterface contents have been copied

4) Execute `python3 -m pip install -r requirements.txt` to install required programs/libraries.

Modifying django-scheduler library to support 12-hour time and associating events with experiments

1) The original django-scheduler library should be installed from the previous steps. You do not have to do all the bower stuff, as jQuery and Bootstrap are in the project already as static files.

2) Find where django-scheduler is installed. For myself, it installed in the /usr/local/lib/python3.6/site-packages/schedule/ directory, but for other computers/OSes it may be different.

3) Replace its contents with the contents in the zip file found in FrontInterface/docs/build/html/\_downloads/django-scheduler-modified.zip

Front Interface installation is now complete. If database issues occur try `manage.py makemigrations` then `manage.py migrate` using the file found within FrontInterface/trucksite.

### Installation of the Network Control Nodes (BeagleBones with TruckCape)

1) Install Python3 on the BeagleBone

2) Install the contents of NetworkControlNodes/CANLogger/OldSystem into a directory for the old logging system and the contents of NetworkControlNodes/CANLogger/NewSystem into a seperate directory for the new logging system. This is to be installed on the BeagleBone used for logging CAN traffic.

3) Install the contents of NetworkControlNodes/ExperimentHandler into a directory for the experiment handler. This is to be installed on a seperate BeagleBone than step 2 to function as the experiment handler. In the original context, this was connected to the Smart Sensor Simulator 2 connected to a brake controller over USB.

4) Install the contents of NetworkControlNodes/SimulatorController into a directory for the simulator controller. This is to be installed on a sperate BeagleBone than step 2 and 3 to function as a simulator controller. In the original context, this was connected to the Smart Sensor Simulator 2 connected to an engine controller over USB.

5) Execute `python3 -m pip install -r requirements.txt` to install required programs/libraries for both the experiment handler and the old logging system for the CAN Logger within the directories where they were installed.

Network Node installation is now complete.

### Installation of the Smart Sensor Simulator 2 Software

1) Use Arduino https://www.arduino.cc/ with the teensyduino Add-In https://www.pjrc.com/teensy/teensyduino.html to upload the ino file found within SensorSimulation/. Make sure to have the entire folder contents in the same directory when uploading.

Simulation software installation is now complete.

## Running Testbed with Old Logging System for "Live Data"
Must be used for "Live Data" functionality of the testbed (Chapter 3 material)

1) On the front interface, open a terminal and navigate to the directory where the front interface source code was installed. Navigate inside the trucksite directory.

2) On the front interface run `python3 manage.py runserver 0.0.0.0:portNumber` where portNumber is the decimal number of the port you want to host the webserver on. Note: The IP 0.0.0.0 tells the program to host the server on the IP of the machine it is running on. The front interface server is now online.

3) On the CAN Logger, navigate to where the old logging system was installed and execute `python3 app.py`

4) On the Experiment Handler, navigate to where the experiment handler source code was installed and execute `python3 app.py`

5) On the Simulator Controller, navigate to where the simulator controller source code was installed and execute `python3 engineServer.py`

6) Ensure the ignition of the brake controller is on, either by sending a serial command from the BeagleBone connected to it (the experiment handler) or manually turning it on

The Live Data and Live Plots webpages should be operational as intended with the ability to simulate front axle speed as well as perform some network based attacks as documented in the thesis.

## Running Testbed with New Logging System
Must be used for experimentation to ensure 100% logging success at 100% busload and 250kbps bitrate (testing from Chapter 4)

1) On the front interface, open a terminal and navigate to the directory where the front interface source code was installed. Navigate inside the trucksite directory.

2) On the front interface run `python3 manage.py runserver 0.0.0.0:portNumber` where portNumber is the decimal number of the port you want to host the webserver on. Note: The IP 0.0.0.0 tells the program to host the server on the IP of the machine it is running on. The front interface server is now online.

3) On the front interface, open a second terminal and navigate to the directory where the front interface source code was installed. Navigate inside the trucksite directory.

4) On the front interface run `python3 manage.py process_tasks`. The front interface can now perform background database tasks for data transfer within an experiment.

5) On the CAN Logger, navigate to where the new logging system was installed and execute `python3 tcpCANServer.py`

6) On the Experiment Handler, navigate to where the experiment handler source code was installed and execute `python3 app.py`

7) On the Simulator Controller, navigate to where the simulator controller source code was installed and execute `python3 engineServer.py`

Ensure that the machine timestamps on each of the BeagleBones an front interface are correct, and the testbed should be operational. Experiments can be created/scheduled/graphed/downloaded.

# Useful Links

1) TruckCape Schematics/GitHub: https://github.com/Heavy-Vehicle-Networking-At-U-Tulsa/TruckCapeProjects

2) Smart Sensor Simulator 2 Schematics/GitHub: https://github.com/jeremy-daily/SSS2

3) TCPCAN Data Transfer GitHub: https://github.com/Heavy-Vehicle-Networking-At-U-Tulsa/TruckCapeProjects/tree/master/SocketCAN