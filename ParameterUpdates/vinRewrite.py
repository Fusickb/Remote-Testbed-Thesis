import os, time
import binascii


id = "18DA00F1"
defaultPart = "10142EF1A0" #14 is size, 2E is read, F1A0 is vin id
vinNumber = input("VIN: ")
while len(vinNumber) < 17:
    vinNumber += " "
vinBytes = binascii.hexlify(vinNumber.encode())
vinFirst = defaultPart + vinBytes[0:6].decode()
vinSecond = "21" + vinBytes[6:20].decode()
vinThird = "22" + vinBytes[20:].decode()
print("Rewriting the VIN on the CPC4 ECM")
front = "cansend can1 " + id + "#"
os.system(front + vinFirst)
#time.sleep(0.035)
os.system(front + vinSecond)
#time.sleep(0.035)
os.system(front + vinThird)
