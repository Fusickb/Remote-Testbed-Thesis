import os, time
from canTools import CanTools
import binascii

def calculateKey(seed):
	m = 1
	b = 1 #m and b set to 1 for security concerns
	key = (m*(int(seed,16)-1)+b) % 65536
	return key

bus = CanTools('can1')
updated = False
engineId = '18DAF100'
"""vinNumber = input("VIN: ")
defaultPart = "10142EF1A0"
while len(vinNumber) < 17:
	vinNumber += " "
vinBytes = binascii.hexlify(vinNumber.encode())
vinFirst = defaultPart+vinBytes[0:6].decode()
vinSecond = "21" + vinBytes[6:20].decode()
vinThird = "22" + vinBytes[20:].decode()
front = "cansend can1 " + engineId + "#"""

os.system('cansend can1 18DA00F1#0227050000000000')

while True:
	message = bus.readMessage()
	if message[0] == engineId and message[1][:6] == '046705':
		seed = message[1][6:10]
		key = calculateKey(seed)
		keyMsg = "cansend can1 18DA00F1#042706{:04X}000000".format(key)
		if seed != "0000":
			os.system(keyMsg)
		"""time.sleep(.2)
		print("Rewriting the VIN on the CPC4 ECM")
		os.system(front+vinFirst)
		os.system(front+vinSecond)
		os.system(front+vinThird)"""
