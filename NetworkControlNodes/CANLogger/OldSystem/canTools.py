import socket, struct, time, sys
class CanTools():
    def __init__(self, interface):
        self.interface = interface
        self.canSocket = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        self.canSocket.settimeout(1)
        try:
            self.canSocket.bind((interface,))
        except OSError:
            sys.stderr.write("Could not bind to interface '%s'\n" % interface)
        self.format = '<IB3x8s'
    
    def readMessageRaw(self):
        return self.canSocket.recv(16) #receive 16 bytes
    def readMessage(self):
        canPacket = self.readMessageRaw()
        canId, length, data = struct.unpack(self.format, canPacket)
        canId &= socket.CAN_EFF_MASK
        canId = ('%02x' % canId).upper()
        while len(canId) < 8:
            canId = '0' + canId
        data = data[:length].hex().upper()
        n = 2
        data = [data[i:i+n] for i in range(0,len(data),n)]
        message = (self.interface + " " + canId + " [" + str(length) +"] " + " ".join(data))
        return message

    def readWheelBasedVehicleSpeed(self):
        vehicle_speed = 0
        while(True):
            canPacket = self.readMessageRaw()
            canId, length, data = struct.unpack(self.format, canPacket)
            canId &= socket.CAN_EFF_MASK
            canId = ('%02x' % canId).upper()
            while len(canId) < 8:
                canId = '0' + canId
            if canId[2:8] == 'FEF100':
                data = data[:length].hex().upper()
                n = 2
                data = [data[i:i+n] for i in range(0, len(data), n)]
                bytes = str(data[1] + data[0])
                vehicle_speed = int(data[2] + data[1], 16)/256
                break
        return vehicle_speed
        
    def readBytes(self, canPacket):
        canId, length, data = struct.unpack(self.format, canPacket)
        canId &= socket.CAN_EFF_MASK
        canId = ('%02x' % canId).upper()
        while len(canId) < 8:
            canId = '0' + canId
        data = data[:length].hex().upper()
        n = 2
        data = [data[i:i+n] for i in range(0, len(data),n)]
        print("  " + self.interface, end="  ")
        print(canId + "  [" + str(length) +"]", end = "  ")
        print(" ".join(data))

    def sendMessage(self, canId, payload):
        if len(str(canId)) > 3:
            self.canId = canId | 1 << 31 #This sets IDE Flag
        else:
            self.canId = canId
        self.canPacket = struct.pack(self.format, self.canId, len(payload), payload)
        #self.canSocket.send(b"\x00\x01\xf0\x88\x08\x00\x00\x00\x11\x22\x33\x44\x55\x66\x77\x88")
        self.canSocket.send(self.canPacket)

if __name__ == "__main__":
    #myNet = CanTools(interface)
    myNet = CanTools('can1')
    net2 = CanTools('can0')
    while True:
        print (myNet.readMessage())
        print (net2.readMessage())
        time.sleep(.1)
