import socket,sys

sock = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
sock.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_ERR_FILTER, socket.CAN_ERR_MASK)

interface='can1'

try:
    sock.bind((interface,))
except OSError:
    print("Could not bind to interface '%'\n" % interface)
    sys.exit()

counter = 0
while True:
    if  sock.recv(16):
        counter+=1
        print("RX Count:", counter)
