

from canTools import CanTools
from NSFutilities import *
from multiprocessing import Process, Queue
import arrow



class NSFlogger:
    def __init__(self, **kwargs):
        self.q = Queue(maxsize=0)
        try:
            self.blocksize = kwargs.get('blocksize')
            self.interface = kwargs.get('interface')
        except KeyError:
            print ('KeyError on NSFlogger constructor')
    def push(self, q):
        block = []
        while self.running:
            if q.qsize() > self.blocksize and len(block)<self.blocksize:
                block.append(q.get())
            elif len(block) == self.blocksize:
                log_message_block(block)
                print('send block')
                del block[:]
            else:
                pass
    def listen(self, bus, q):
        while True:
       	    message = bus.readMessage()
            utc = arrow.utcnow()
            timestamp = '.'.join((str(utc.timestamp),str(utc.microsecond)))
            q.put((timestamp, message))

    def start(self):
        try:
            bus = CanTools(self.interface)
        except OSError:
            print('Could not bind to interface {}'.format(self.interface))
        self.running = True
        Process(target=self.listen, args=(bus,self.q)).start()
        Process(target=self.push, args=(self.q,)).start()

    def stop(self):
        self.running = False

if __name__ == '__main__':
    can0 = NSFlogger(blocksize=1000, interface='can0')
    can1 = NSFlogger(blocksize=1000, interface='can1')
    can1.start()
    can0.start()
