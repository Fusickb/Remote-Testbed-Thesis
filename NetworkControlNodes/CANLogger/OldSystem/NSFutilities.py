import arrow
import datetime
import MySQLdb
import tempfile, heapq, os
from itertools import islice
import json
import subprocess
import shutil
from NSFsettings import *
#import matplotlib.pyplot as plt

BASE_TIMESTAMP = arrow.get(datetime.datetime(year=2017, month=1, day=1)).timestamp

class LogGen():
    def __init__(self, experimentname):
        global SLICE_INDEX
        self.N = 800000
        self.start = 0
        self.experimentname = experimentname
        self.loglocation = ''.join([LOG_DIR, self.experimentname, "_log"])
        with open(self.loglocation, 'rt') as logfile:
                self.lc = linecount(logfile)
        self.chunkable = (self.N < self.lc)
        sortedtfs = self.get_experiment_data()
        opentfs = [iter(open(n).readline, "") for n in sortedtfs]
        if len(opentfs) > 1:
            self.sortedcompletelog = heapq.merge(*opentfs, key=lambda line: float(line.split(',')[0]))
        elif len(opentfs) == 1:
            self.sortedcompletelog = opentfs[0]

    def nextJSON(self, sliceindex):
        if (sliceindex-1)*self.N > self.lc:
            raise tornado.web.HTTPError(404)
        if self.chunkable:
            jsonobj = dict(log=[], hasmore=self.hasNext(sliceindex))
        else:
            jsonobj = dict(log=[], hasmore=False)
        for counter in range(0,self.N):
            try:
                line = next(self.sortedcompletelog)
            except StopIteration:
                break
            jsonobj['log'].append([float(line.split(',')[0]), line.split(',')[1]])
        if not self.hasNext(sliceindex):
            shutil.rmtree('experimentlogs/{}'.format(self.experimentname))            
        print(jsonobj)
        return json.dumps(jsonobj)

    def hasNext(self, sliceindex):
        if (sliceindex*self.N) < self.lc:
            return True
        else: return False

    def get_experiment_data(self):
        global BASE_TIMESTAMP
        if self.chunkable:
            iters = []
            if not os.path.isdir("experimentlogs/{}".format(self.experimentname)):
                subprocess.run(['mkdir', 'experimentlogs/{}'.format(self.experimentname)])
            if not os.path.isdir("experimentlogs/{}/tmp".format(self.experimentname)):
                subprocess.run(['mkdir', 'experimentlogs/{}/tmp'.format(self.experimentname)])
            counter = 0
            for chunk in range(0,self.lc,self.N):
                with open(self.loglocation,'rt') as logfile:
                    try:
                        outputfile = open('experimentlogs/{}/{}_chunk{}'.format(self.experimentname, self.experimentname, counter), 'w+')
                        subprocess.run(['head','-{}'.format(self.N), self.loglocation], stdout=outputfile)
                        outputfile.close()
                        subprocess.run(['sed', '-n', '-i', '{},$p'.format(self.N+1), self.loglocation], stdout=None)
                        counter+=1
                    except StopIteration: break
                    logfile.close()
            os.remove(self.loglocation)
            logchunknames = next(os.walk('experimentlogs/{}'.format(self.experimentname)))[2]
            sortedtfs = []
            print(logchunknames)
            for filename in logchunknames:
                print('reading from {}:  size: {}'.format(filename, os.stat('experimentlogs/{}/{}'.format(self.experimentname,filename))))
                tf = tempfile.NamedTemporaryFile(mode='w+t', delete=False, dir='experimentlogs/{}/tmp'.format(self.experimentname))
                with open('experimentlogs/{}/{}'.format(self.experimentname, filename), 'rt') as chunk:
                    for line in sorted(chunk, key=lambda line: float(line.split(',')[0])):
                        if float(line.split(',')[0]) > BASE_TIMESTAMP:
                            tf.write(line)
                os.remove('experimentlogs/{}/{}'.format(self.experimentname, filename))    
                sortedtfs.append(tf.name)
                tf.close()
            return sortedtfs
        else:
            with open(self.loglocation, 'rt') as logfile:
                if not os.path.isdir("experimentlogs/{}".format(self.experimentname)):
                    subprocess.run(['mkdir', 'experimentlogs/{}'.format(self.experimentname)])
                if not os.path.isdir("experimentlogs/{}/tmp".format(self.experimentname)):
                    subprocess.run(['mkdir', 'experimentlogs/{}/tmp'.format(self.experimentname)])
                tf = tempfile.NamedTemporaryFile(mode='w+t', delete=False, dir='experimentlogs/{}/tmp'.format(self.experimentname))
                sortedlog = sorted(iter(logfile.readline, ''), key=lambda line:float(line.split(',')[0]))
                for line in sortedlog:
                    if float(line.split(',')[0]) > BASE_TIMESTAMP:
                        tf.write(line)
                tf.close()
                return [tf.name]
def linecount(f):
    for i, l in enumerate(f):
        pass
    return i+1



def get_axle_based_vehicle_speed(experimentname):
    x_values, y_values = [], []
    global BASE_TIMESTAMP
    for line in open('experimentlogs/{}_log'.format(experimentname)):
        timestamp, message = line.split(',')
        parts = message.split(' ')
        if parts[1][2:] == 'FEBF0B' and float(timestamp) > BASE_TIMESTAMP:
            x_values.append(timestamp)
            y_values.append((int(parts[4]+parts[3], 16)/256)*0.621371)
    data = {
        'x_values': x_values,
        'y_values': y_values

    }
 #   plt.plot(x_values, y_values)
 #   plt.show()
    return data
    
def get_gov_speed(experimentname):
    x_values, y_values = [], []
    global BASE_TIMESTAMP
    for line in open('experimentlogs/{}_log'.format(experimentname)):
        timestamp, message = line.split(',')
        parts = message.split(' ')
        if parts[1] == '18DAF100' and float(timestamp) > BASE_TIMESTAMP:
            if parts[3] == '26':
                x_values.append(timestamp)
                y_values.append(float(int(parts[4] + parts[5], 16))*.0049)
                
    data = {'x_values': x_values, 'y_values': y_values}
    return data

def get_vin(experimentname):
    x_values, y_values = [], []
    global BASE_TIMESTAMP
    vinResponse = '101462F1A0'
    vin = ''
    vinCount = 0
    for line in open('experimentlogs/{}_log'.format(experimentname)):
        timestamp, message = line.split(',')
        parts = message.split(' ')
        if parts[1] == '18DAF100' and float(timestamp) > BASE_TIMESTAMP:
            fiveBytes = ''
            fiveBytes = ''.join(parts[3:8])
            if fiveBytes == vinResponse:
                vin += ''.join(parts[8:])[:-1]
                vinCount += 1
            elif vinCount == 1 and parts[3] == '21':
                vin += ''.join(parts[4:])[:-1]
                vinCount += 1
            elif vinCount == 2 and parts[3] == '22':
                vin += ''.join(parts[4:])[:-1]
                x_values.append(timestamp)
                y_values.append(vin)
                vin = ''
                vinCount = 0
    print(str(len(x_values)) + ' VIN timestamps recorded')
    data = {
        'x_values': x_values,
        'y_values': y_values

    }
    
    return data
