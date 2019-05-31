

from canTools import CanTools
from NSFutilities import *
import arrow
import logging
import datetime
import time
import requests
from tornado import gen
from socket import timeout
from multiprocessing import Process, Manager
from websocket import create_connection


class LogHandler():
    def __init__(self):
        self.loggers = []
        self.experimentname = 'Not Assigned'
        self.endTime = None
    def setExperimentName(self, experimentname):
        self.experimentname = experimentname
    def setEndTime(self, time):
        self.endTime = time
    @gen.coroutine
    def start_logging(self):
        print('start_logging, loggers = ' + str(self.loggers))
        f = open('experimentlogs/{}_log'.format(self.experimentname), 'w+')
        can0 = NSFlogger(interface='can0')
        can1 = NSFlogger(interface='can1')
        print('created/opened log file in experimentlogs')
        _0 = Process(target=can0.start, args=(f, self.endTime))
        _1 = Process(target=can1.start, args=(f, self.endTime))    
        _0.start()
        _1.start()
        print('started logger')
    
class NSFlogger():
    def __init__(self, **kwargs):
        try:
            self.interface = kwargs.get('interface')
            self.buffer = []
        except KeyError:
            print ('KeyError on NSFlogger constructor')

    @gen.coroutine
    def start(self, file, endTime):
        try:
            bus = CanTools(self.interface)
            print('binded to '+self.interface)
        except OSError:
            print('Could not bind to interface {}'.format(self.interface))
        print(datetime.datetime.now(), endTime)
        print(datetime.datetime.now() < endTime)
        print(datetime.datetime.now().timestamp)
        while(datetime.datetime.now() < endTime):
            try:
                message = bus.readMessage()
                time = arrow.utcnow()
                file.write(str(time.timestamp + (time.microsecond/1e6)) + ',' + message + '\r\n')            
            except timeout as t:
                print('handled timeout')
                pass
        file.close()

    """@gen.coroutine
    def stream(self):
        try:
            bus = CanTools(self.interface)
            print('Streaming from', self.interface)
            startTime = time.time()
        except OSError:
            print('Could not stream from interface {}'.format(self.interface))
        while True:
            try:
                currentTime = time.time()
                message = bus.readMessage()
                self.buffer.append(message)
                if len(self.buffer) > 8000 or currentTime-startTime > 1:
                    startTime = time.time()
                    sendingBuffer = self.buffer
                    self.buffer = []
                    jsonData = {'data': sendingBuffer}
                    print("Trying to send")
                    Process(target=self.sendJson, args=(jsonData)).start()
                    print("Sent a JSON of data")

                print(message)
            except timeout as t:
                pass"""

    #@gen.coroutine
    def stream(self): #, sendBuffer):
        #print("Type:",type(sendBuffer))
        try:
            bus = CanTools(self.interface)
            print('Streaming from', self.interface)
            currentMsgs = []
        except OSError:
            print('Could not stream from interface {}'.format(self.interface))
        currentTime = arrow.utcnow()
        startTime = arrow.utcnow()
        currentMsgs = []
        while (currentTime.timestamp+currentTime.microsecond/1e6) - (startTime.timestamp+startTime.microsecond/1e6) <= 0.25:
            currentTime = arrow.utcnow()
            message = bus.readMessage()
            print(message)
            currentMsgs.append((str(currentTime.timestamp + (currentTime.microsecond/1e6)),message)) #appends a tuple of timestamp and message

        print("currentMsgs", currentMsgs)
        return currentMsgs
        """while True:
            currentTime = arrow.utcnow()
            message = bus.readMessage()
            currentMsgs.append((str(currentTime.timestamp + (currentTime.microsecond/1e6)),message)) #appends a tuple of timestamp and message
            if (currentTime.timestamp+currentTime.microsecond/1e6) - (startTime.timestamp+startTime.microsecond/1e6) > 0.25: #if it's been 250ms
                print("trying to update proxy")
                manager = Manager()
                proxy = manager.list(sendBuffer)
                print("created proxy")
                proxy.append(currentMsgs)
                print("appended to proxy")
                sendBuffer=currentMsgs
                print("success", type(sendBuffer))
                currentMsgs = []
                startTime = arrow.utcnow()
                print("Transferring Data")"""

            #if it's been xx ms, update the shared variable

    def updateSharedDict(self, dictionary):
        dictionary

    def sendJson(self, data):
        websocket = create_connection('ws://129.244.254.147:11111')
        websocket.send(data)
        print(data)
        
        #requests.post("http://129.244.254.147:23456/experimenteditor/livedata/", json=data, timeout=0.01)

