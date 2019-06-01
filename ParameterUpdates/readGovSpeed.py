from canTools import CanTools

def calcSpeed(govBits):
	return govBits*0.0049

needingGovSpeed = True
bus = CanTools('can1')
id = 0x18DA00F1
readReq = b'\x03\x22\x02\x03\x00\x00\x00\x00'
bus.sendMessage(id,readReq)

while needingGovSpeed:
	message = bus.readMessage()
	if message[0] == '18DAF100' and message[1][:2] == '26':
		govBits = int(message[1][2:6],16)
		govSpeed = calcSpeed(govBits)
		print("{0:.4f} mph".format(govSpeed))
		needingGovSpeed = False

