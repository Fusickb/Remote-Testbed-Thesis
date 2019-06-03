# Overview

Source code for the Network Control Nodes of the Remote Access Testbed. Each node is a BeagleBone Black https://beagleboard.org/black with TruckCape https://github.com/Heavy-Vehicle-Networking-At-U-Tulsa/TruckCapeProjects.

1) CANLogger
	- Contains both the Old Logging System and the New Logging System (After Validation Testing)
	- Responsible for recording vehicle messages or transferring the data to the front interface

2) ExperimentHandler
	- Contains source code for the experiment handler
	- Responsible for scheduling commands (simulation commands, starting logging for old system, sending CAN frames)
	- Responsible for sending serial commands to Smart Sensor Simulator 2 connected to brake controller for brake simulation commands

3) SimulatorController
	- Contains source code for the simulator controller
	- Responsible for simulation commands for additional electronic control unit (engine in this case)
	- Only implemented ignition