import os, binascii, time 
from canTools import CanTools

vinCount = 0

bus = CanTools("can1")
id = "18DA00F1"
data = "0322F1A000000000"
vinResponse = '101462F1A0'

#os.system("cansend can1 " + id + "#" +data)
while(vinCount < 3):
	os.system("cansend can1 " + id + "#" + data)
	message = bus.readMessage()#list with id and data as strings
	if(message[0] == '18DAF100'): #if from engine to diag tool	
		if(message[1][:10]==vinResponse):
			vinPart1 = bytes.fromhex(message[1][10:]).decode("ascii")
			#print(vinPart1)
			vinCount += 1
		elif(vinCount == 1 and message[1][:2] == "21"):
			#print("Part2")
			vinPart2 = bytes.fromhex(message[1][2:]).decode("ascii")
			vinCount += 1
		elif(vinCount == 2 and message[1][:2] == "22"):
			#print("Part3")
			vinPart3 = bytes.fromhex(message[1][2:]).decode("ascii")
			vinCount += 1
		if(vinCount == 3):
			print("Reading VIN from CPC4 ECM...")
			print("VIN: " + vinPart1+vinPart2+vinPart3)
			"""time.sleep(2)
			vinPart1 = ""
			vinPart2 = ""
			vinPart3 = ""
			vinCount = 0
			"""
