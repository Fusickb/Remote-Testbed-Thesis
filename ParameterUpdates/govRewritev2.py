import os,time
from canTools import CanTools

def calcBits (govSpeed):
    return round(float(govSpeed)*204.0816)

id = 0x18DA00F1
engineId = '18DAF100'
bus = CanTools('can1')
updateMsgs = []

speed = -1
govSpeed = input("Set Governor Speed to (mph): ")
govBytes= "{:04X}".format(calcBits(govSpeed))
govUpdate = bytes.fromhex(govBytes)
fullBytes = b'\x26' + govUpdate + b'\x61\xFB\x1F\x40\x05'

updateMsgs.append(b'\x21\x00\x83\x40\x1F\x40\x06\x40')
updateMsgs.append(b'\x22\x00\x0F\x00\xA0\x00\x01\x2C')
updateMsgs.append(b'\x23\x16\x00\x00\x0F\x38\x40\x00')
updateMsgs.append(b'\x24\x40\x2C\x56\x24\x34\x04\x00')
updateMsgs.append(b'\x25\x15\xE0\x7D\x00\x0F\x61\xA8')
updateMsgs.append(fullBytes) 
updateMsgs.append(b'\x27\x00\x12\xC0\x11\x30\x03\x20')
updateMsgs.append(b'\x28\x01\x00\x00\x80\x00\x00\x00')
updateMsgs.append(b'\x29\x01\x00\x04\x00\x00\x01\x00')
updateMsgs.append(b'\x2A\x34\x80\xFF\xFF\xFF\xFF\xFF')
updateMsgs.append(b'\x2B\xFF\xFF\x01\x12\xC0\x00\x00')
#updateMsgs.append(b'\x02\x27\x05\x00\x00\x00\x00\x00')


passed = False
#bus.sendMessage(id,b'\x04\x31\x03\x04\x01\x00\x00\x00')
bus.sendMessage(id,b'\x10\x52\x2E\x02\x03\x03\x1C\x20')
for data in updateMsgs:
    bus.sendMessage(id,data)