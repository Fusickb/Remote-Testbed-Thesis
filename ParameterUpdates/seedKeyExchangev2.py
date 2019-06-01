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
id = 0x18DA00F1
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
	if message[0] == engineId:
		if message[1][:6] == '046705':
			seed = message[1][6:10]
			key = calculateKey(seed)
			keyMsg = "cansend can1 18DA00F1#042706{:04X}000000".format(key)
			if seed != "0000":
				os.system(keyMsg)
				print("Key Sent")
		elif message[1][:6] == '026706':
			bus.sendMessage(id,b'\x10\x0D\x2E\xF1\x5C\x00\x00\x00')
		elif message[1][:6] == '300814':
			bus.sendMessage(id,b'\x21\x12\x03\x09\xC5\x0B\x82\x84')
		"""time.sleep(.2)
		print("Rewriting the VIN on the CPC4 ECM")
		os.system(front+vinFirst)
		os.system(front+vinSecond)
		os.system(front+vinThird)"""
