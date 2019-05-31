import tornado.web
import datetime
from tornado import httpclient
from tornado.web import MissingArgumentError
from jinja2 import Environment, FileSystemLoader
import tornado.web
import os, os.path
import wtforms
from wtforms_tornado import Form
import urllib
import random
import string
import re
import sys, inspect
import requests
import arrow
import time
import csv
import json
from NSFsettings import *
from NSFutilities import *
from NSFlogger import LogHandler,NSFlogger
from tornado import gen
from multiprocessing import Process, Manager
from tornado.escape import json_encode

CURRENT_EXPERIMENT_LOGGEN = None

class BaseHandler(tornado.web.RequestHandler):

    def set_default_headers(self):
        print ("setting headers!!!")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')

    def options(self):
        # no body
        print("optionssss")
        self.set_status(204)
        self.finish()

class DeleteExperiment(tornado.web.RequestHandler):
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    def post(self, experimentname):
        os.remove("experimentlogs/{}".format(experimentname))
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()


class GetExperiment(tornado.web.RequestHandler):
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    def get(self, experimentname, slnum):
        global CURRENT_EXPERIMENT_LOGGEN
        self.set_header('Content-Type', 'attachment; filename="{}_log.csv"'.format(experimentname))
        if CURRENT_EXPERIMENT_LOGGEN == None or CURRENT_EXPERIMENT_LOGGEN.experimentname != experimentname:
            CURRENT_EXPERIMENT_LOGGEN = LogGen(experimentname)
            print('created new loggen object')
        self.write(CURRENT_EXPERIMENT_LOGGEN.nextJSON(int(slnum)))
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()

class GetAxleBasedVehicleSpeed(tornado.web.RequestHandler):
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    def get(self, experimentname):
        self.write(get_axle_based_vehicle_speed(experimentname))
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()

class GetVin(tornado.web.RequestHandler):
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    def get(self, experimentname):
        self.write(get_vin(experimentname))
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()

class GetGovSpeed(tornado.web.RequestHandler):
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    def get(self, experimentname):
        self.write(get_gov_speed(experimentname))
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()

class StartLogging(tornado.web.RequestHandler):
    def initialize(self, **kwargs):
        self.context = kwargs
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    @gen.coroutine
    def post(self, **kwargs):
        request = self.request
        loghandler = self.context['loghandler']
        postdata = json.loads(str(request.body, 'utf-8'))
        experimentname = postdata['experimentname']
        endTime = datetime.datetime.fromtimestamp(postdata['endtime'])
        loghandler.setExperimentName(experimentname)
        loghandler.setEndTime(endTime)
        yield loghandler.start_logging()
        return

    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()

class StartCanStream(tornado.web.RequestHandler):
    #streamStates = {'can0':0, 'can1':0}
    #manager = Manager()
    #startTime = None
    #sendingBuffer0 = manager.list([]) #one for each interface
    #sendingBuffer1 = manager.list([])
    #processes = {'can0': Process(target=NSFlogger(interface='can0').stream, args=(sendingBuffer0,)),
    #             'can1': Process(target=NSFlogger(interface='can1').stream, args=(sendingBuffer1,))}

    #startStream = True
    def initialize(self, **kwargs):
        self.context = kwargs
        print(self.request)
    def set_default_headers(self):
        print("setting headers")
        self.set_header("Access-Control-Allow-Origin", "*")
        self.set_header("Access-Control-Allow-Headers", "x-requested-with")
        self.set_header("Access-Control-Allow-Methods", "GET")
    """@gen.coroutine
    def post(self, **kwargs):
        request = self.request
        loghandler = self.context['loghandler']
        postdata = json.loads(str(request.body, 'utf-8'))
        print(postdata)
        interface = postdata['interface']
        StartCanStream.streamStates[interface] = postdata['streamstate']
        if StartCanStream.streamStates[interface]:
            try:
                StartCanStream.processes[interface].start()
                print("I'm streaming",interface)
            except Exception as e:
                print("Couldn't start process\n",e)
        else:
            try:
                StartCanStream.processes[interface].terminate()
                StartCanStream.processes[interface] = Process(target=NSFlogger(interface=interface).stream)
                print("Done Streaming",interface)
            except:
                print("Couldn't end process")"""

    """@gen.coroutine
    def get(self, **kwargs):
        #get current timestamp
        #if first get start stream on can0 and can1
        sendingBuffer = []
        print("Got a GET request")
        print(StartCanStream.startStream)
        if StartCanStream.startStream: #if stream needs to be started
            StartCanStream.processes['can0'].start()
            StartCanStream.processes['can1'].start()
            StartCanStream.startStream = False
            StartCanStream.startTime = arrow.utcnow().timestamp
            print("started streams")
            time.sleep(1)
        currentTime = arrow.utcnow().timestamp
        print("Send Buf1:",StartCanStream.sendingBuffer1)
        sendingBuffer = StartCanStream.sendingBuffer0+StartCanStream.sendingBuffer1
        jsonToSend = {'data': sendingBuffer}
        print("About to send")
        self.write(jsonToSend)
        print("SENDING JSON MUAHAHAHAHA")

        #else isn't really used, erase maybe? might use later
        else:
            print("made it in the else")
            StartCanStream.startStream = True
            StartCanStream.processes['can0'].terminate()
            StartCanStream.processes['can1'].terminate()
            StartCanStream.processes['can0'] = Process(target=NSFlogger(interface='can0').stream, args=sendingBuffers0)
            StartCanStream.processes['can1'] = Process(target=NSFlogger(interface='can1').stream, args=sendingBuffers1)
            print("Stopping stream")"""
        #after 5 seconds of no get, stop streams
        #send buffer and empty buffer everytime a get is received, empty first time or delay?
    @gen.coroutine
    def get(self, **kwargs): #no continuous stream, just record/send on any get
        callback = self.get_argument("callback")
        print("Callback arg:" + callback)
        print("starting logger")
        messages = NSFlogger(interface='can1').stream()
        print("about to send")
        #jsonToSend = {'data': messages}
        #print(jsonToSend)
        jsonp = "{jsfunc}({json});".format(jsfunc=callback, json=json_encode({"data": messages}))
        #self.set_header("Access-Control-Allow-Origin","http://129.244.254.147:23456/experimenteditor/livedata")
        self.finish(jsonp)
        print("Sending messages")

    @gen.coroutine
    def options(self, **kwargs):
        print("optionssss")
        self.set_status(204)
        self.finish()
