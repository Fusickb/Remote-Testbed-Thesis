import serial
from tornado import gen
import time
import requests, json
serial = serial.Serial('/dev/ttyACM0')
CURRENT_AXLE_SPEED = 0
ON_MSG = json.dumps({'command': 'on'})
OFF_MSG = json.dumps({'command': 'off'})
def InitializeTruck():
    requests.post('http://129.244.254.23:8080', data=ON_MSG)
    serial.write('50,1'.encode('ASCII'))
    time.sleep(1)
    serial.write('17,2035'.encode('ASCII'))
    time.sleep(1)
    serial.write('18,2035'.encode('ASCII'))
    time.sleep(1)
    serial.write('35,2048'.encode('ASCII'))
    time.sleep(1)
    serial.write('36,2048'.encode('ASCII'))
    time.sleep(1)
    serial.write('83,14'.encode('ASCII'))
    print('done intializing')


@gen.coroutine
def EndExperiment():
    serial.write('50,0'.encode('ASCII'))
    return

@gen.coroutine
def TurnIgnitionOn():
    requests.post('http://129.244.254.23:8080', data=ON_MSG)
    serial.write('50,1'.encode('ASCII'))

@gen.coroutine
def TurnIgnitionOff():
    requests.post('http://129.244.254.23:8080', data=OFF_MSG)
    serial.write('50,0'.encode('ASCII'))

@gen.coroutine
def SetAxleBasedVehicleSpeed(speedMPH):
    print('83,{}'.format(MPHtoFREQ(speedMPH)).encode('ASCII'))
    serial.write('83,{}'.format(MPHtoFREQ(speedMPH)).encode('ASCII'))
    global CURRENT_AXLE_SPEED
    CURRENT_AXLE_SPEED = speedMPH

def MPHtoFREQ(MPH):
    return (MPH+.0383)/.0716

@gen.coroutine
def SetBrakePressure(percent, duration):
    NewAxleSpeed = (-1*(percent/100)*78999*(duration/3600))+CURRENT_AXLE_SPEED
    string = 'ACC,83,{},{},0'.format(MPHtoFREQ(NewAxleSpeed), duration)
    serial.write(string.encode('ASCII'))
    print(string)

class AnalogOut: #class for setting pins from user input values
    def __init__(self, pinNumber, name):
        self.pinNumber = int(pinNumber)
        self.name = name
        self.getSettings(True)

    def getSettings(self, ifPrinting): #sets instance variable from values in file
        fileData = []
        with open("pinSettings.csv","r") as f: #get file data
            line = f.readline()
            fileData.append(line)
            while line:
                line = f.readline()
                fileData.append(line) 
        self.settings = fileData[self.pinNumber].split(",")
        if ifPrinting:
            print(self.settings)

    def getPinNumber(self):
        return self.pinNumber

    def getName(self):
        return self.name

    def setValue(self, userInputValue): #sets simulation value
        self.getSettings(False)
        bits = float(userInputValue) * float(self.settings[2]) * (1/float(self.settings[4])) # converts user input value to number of bits
        print(bits)
        settingValue = int(bits) * float(self.settings[6]) + float(self.settings[7]) #converts to value to set pin. bits is int since partial bits doesn't make sense
        serialCommand = (str(self.pinNumber) + ',' + str(settingValue)).encode('ASCII')
        print(serialCommand)
        serial.write(serialCommand)
