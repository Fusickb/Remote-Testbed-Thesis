from background_task import background
from django.contrib.auth.models import User
from . import models	
from django.utils import timezone
from django.core.mail import send_mail
from datetime import timedelta
import arrow
import requests
from pprint import pprint
from notifications.signals import notify
import socket
from multiprocessing import Process
import struct

remindermessage = ''
'You have a testbed experiment that is scheduled to run {start_humanized} and ends {end_humanized}.'
'When it has finished running (or its specified timeslot is finished), we will send you link (WIP) to the data you have requested.'
''
'--The TU Testbed Experiment Team'

LOG_DIRECTORY = 'testbedLogs/'

expfinishedmessage = ''
'A testbed experiment that you scheduled named {title} that finished is now available to view.'
''
'Click this link to view your results: {link}'
''
'--The TU Testbed Experiment Team'

ENGINEBEAGLEBONEIP='http://129.244.254.22:8080/api/postexperiment/'
LOGGERIP='http://129.244.254.21:8080/log/api/getExperiment/{expname}/plotdata/axlebasedvehiclespeed/'
VINIP='http://129.244.254.21:8080/log/api/getExperiment/{expname}/plotdata/vin/'
GOVIP='http://129.244.254.21:8080/log/api/getExperiment/{expname}/plotdata/govspeed/'

FULLLOGIP='http://129.244.254.21:8080/log/api/getExperiment/{expname}/{chunkidx}/'

TCP_SERVER_IP = "127.0.0.1"

try:
	TCP_SERVER_IP = socket.gethostbyname(socket.gethostname())
except Exception as e:
	print("Could not get this machine's IP address, defaulting to 127.0.0.1")

TCP_CONTROL_IP = "129.244.254.21"
TCP_CONTROL_PORT = 2319

# The original CAN frame structure and the sockaddr structure are defined
#   in include/linux/can.h:
#     struct can_frame {
#             canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
#             __u8    can_dlc; /* frame payload length in byte (0 .. 8) */
#             __u8    __pad;   /* padding */
#             __u8    __res0;  /* reserved / padding */
#             __u8    __res1;  /* reserved / padding */
#             __u8    data[8] __attribute__((aligned(8)));
#     };

# The implemented CAN frame structure and the sockaddr structure are defined as
#     struct can_frame {
#             canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
#             __u8    can_dlc; /* frame payload length in byte (0 .. 8) */
#             __u8    __micros0;   /* first byte of microsecond timer */
#             __u8    __micros1;  /* second byte of microsecond timer */
#             __u8    __micros2;  /* third byte of microsecond timer */
#             __u8    data[8] __attribute__((aligned(8)));
#     };   

# To match this data structure, the following struct format can be used, grouping micros and dlc as a 32 bit unsigned long integer
can_frame_format = "<LL8s"
# Unsigned Long Integer (little endian), Unsigned Long Integer (DLC and Micros), eight chars (bytes)
# Note: this is 16 bytes

CAN_EFF_MASK = 0x1FFFFFFF #assigned to remove socketCAN dependencies, values obtained from socketCAN
CAN_EFF_FLAG = 0x80000000
CAN_RTR_FLAG = 0x40000000
CAN_ERR_FLAG = 0x20000000
CAN_ERR_MASK = 0x1FFFFFFF
CAN_SFF_MASK = 0x000007FF #standard CAN format

canPorts = {'can0': 2320, 'can1': 2322} # ports used for receiving CAN servers
intfOrder = ['can0', 'can1'] # order of processing intf since dicts are unordered
BUFFER_SIZE = 1425 #maximum amount of data to receive at once for 89 CAN frames and 1 counter byte 16*89+1
SIZE_OF_CAN_FRAME = 16
COUNTER_OFFSET = 1 #number of bytes to offset from counter byte at beginning of packet
rxProcesses = [None for i in range(len(canPorts))]
MAX_TIMER_VALUE = 0xFFFFFF #max value the timer could possible store (3 bytes)

def rxServer(interface): # method used by multiprocessing to listen for CAN frames over TCP
	tcpSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		tcpSock.bind((TCP_SERVER_IP, canPorts[interface]))
	except:
		print("Could not bind TCP Socket for", canPorts[interface])
	tcpSock.listen(1)
	conn, addr = tcpSock.accept()
	print("Connection Address:", addr)
	prev_microTimer = 0 #stores value of previous timer starting at 0 (start of experiment)
	overallTimer = 0 #stores total elapsed time in seconds

	with open(interface + "_temp.csv", 'w') as file: #opens temp file to log interface to
		while True:
			ethData = conn.recv(BUFFER_SIZE)
			if not ethData: break; #break when stream is closed
			print("Received", int(ethData[0]), "CAN Frames.")
			for i in range(int(ethData[0])):
				#gets individual CAN packet
				can_packet = ethData[SIZE_OF_CAN_FRAME*i + COUNTER_OFFSET: SIZE_OF_CAN_FRAME*i + SIZE_OF_CAN_FRAME + COUNTER_OFFSET]
				try:
					can_id, can_dlc_and_microTimer, can_data = struct.unpack(can_frame_format, can_packet)

					can_dlc = (can_dlc_and_microTimer & 0x000000FF) # last byte is dlc b/c little endian
					can_microTimer = (can_dlc_and_microTimer & 0xFFFFFF00) >> 8 #first three bytes are microseconds timestamp
					
					if can_microTimer >=  prev_microTimer: #no rollover, next time stamp is bigger or equal to previous
						overallTimer += (can_microTimer - prev_microTimer)/ 1e6 #converts to seconds and adds the difference to overall timestamp
					else:
						overallTimer += (MAX_TIMER_VALUE - prev_microTimer + can_microTimer)/ 1e6 #since rollover, add difference between previous to max value and add new time
					
					prev_microTimer = can_microTimer
					extended_frame = bool(can_id & CAN_EFF_FLAG)
					error_frame = bool(can_id & CAN_ERR_FLAG)
					remote_tx_req_frame = bool(can_id & CAN_RTR_FLAG)
					type = '' #null is data frame

					if error_frame:
						can_id &= CAN_ERR_MASK
						type = 'ERROR'
						can_id_string = "{:08X}".format(can_id)
					else:
						if extended_frame:
							can_id &= CAN_EFF_MASK
							can_id_string = "{:08X}".format(can_id)
						else:
							can_id &= CAN_SFF_MASK
							can_id_string = "{:03X}".format(can_id)
					if remote_tx_req_frame:
						type = 'RTR'
						can_id_string = "{:08X}".format(can_id)
					hex_data_print = ' '.join(["{:02X}".format(can_byte) for can_byte in can_data])
				except Exception as e:
					print("Couldn't handle a CAN frame. Maybe partial frame?\n" + str(e))
					print(ethData)
				try:
					file.write("{:12.8f},{},{},{},{},{}\n".format(overallTimer, interface, type, can_id_string, can_dlc, hex_data_print))
				except Exception as e:
					print("Couldn't write a CAN frame to the file. Maybe partial frame?\n"+str(e))
					print(ethData)
		conn.close()
						

@background(schedule=timezone.now())
def send_email_reminder(experimentid):
	experiment = models.Experiment.objects.get(exp_pk=experimentid)
	experiment_si = models.ExperimentSchedulingInfo.objects.get(related_experiment__exp_pk=experimentid)
	user_email = experiment.created_by.email
	experiment_start_str = arrow.get(experiment_si.related_event.start).humanize(timezeone.now())
	experiment_end_str = arrow.get(experiment_si.related_event.end).humanize(timezone.now())
	send_mail('Experiment Reminder', remindermessage.format(start_humanized=experiment_start_str, end_humanized=experiment_end_str), 'tu.tib.testbed@gmail.com', [user_email])

@background(schedule=timezone.now())
def send_experiment_json(experimentid):
	experiment = models.Experiment.objects.get(exp_pk=experimentid)
	commands = models.OneTimeSSSCommand.objects.filter(parent_experiment__exp_pk=experimentid).order_by('delay')
	cancommands = models.CANCommand.objects.filter(parent_experiment__exp_pk=experimentid).order_by('delay')
	cangencommands = models.CANGenCommand.objects.filter(parent_experiment__exp_pk=experimentid).order_by('delay')
	ecuupdates = models.ECUUpdate.objects.filter(parent_experiment__exp_pk=experimentid).order_by('delay')
	schedulinginfo = models.ExperimentSchedulingInfo.objects.get(related_experiment__exp_pk=experimentid)
	sssoncelist = []
	ssslooplist = []
	canoncelist = []
	canutilslist = []
	seedkeylist = []
	if ecuupdates.exists():
		firstdelay = float(ecuupdates.first().delay)
		#add flow control/diagnostic messages at the time of the first update
		flowcontrolargs = ['can1', 44, True, False, '18DA00F1', 8, '3008000000000000', None]
		diagargs = ['can1', 243, True, False, '18DA00F1', 8, '0210030000000000', None]
		canutilslist.append([0.0, "cangen", flowcontrolargs])
		canutilslist.append([0.0, "cangen", diagargs])
		if ecuupdates.filter(update_type=1).exists():
			#add read vin 1s
			vinupdateargs = ['can1', 1000, True, False, '18DA00F1', 8, '0322F1A000000000', None]
			canutilslist.append([float(.5), "cangen", vinupdateargs])
			for vinupdate in ecuupdates.filter(update_type=1):
				update_time = float(vinupdate.delay)
				seedkeylist.append([update_time,1,vinupdate.vin])
		if ecuupdates.filter(update_type=2).exists():
			#add read gov. speed .1s
			govupdateargs = ['can1', 1000, True, False, '18DA00F1', 8, '0322020300000000', None]
			canutilslist.append([float(.5), "cangen", govupdateargs])
			for govupdate in ecuupdates.filter(update_type=2):
				update_time = float(govupdate.delay)
				seedkeylist.append([update_time,2,govupdate.governor_speed])
	for command in commands:
		if command.is_repeated:
			duration = command.repeat_delay * (command.repeat_count - 1)
			ssslooplist.append([float(command.delay), command.commandchoice.split('(')[0], float(command.repeat_delay), float(duration)])
		else:
			sssoncelist.append([float(command.delay), command.commandchoice.split('(')[0]])
		if command.quantity is not None and len(command.quantity.split(',')) > 0:
			quantlist = command.quantity.split(',')
			floatlist = []
			for i in range(len(quantlist)):
				floatlist.append(float(quantlist[i])) 
			if command.is_repeated:
				ssslooplist[-1].append(floatlist)
			else:
				sssoncelist[-1].append(floatlist)
		else:
			if command.is_repeated:
				ssslooplist[-1].append([])
			else:
				sssoncelist[-1].append([])
	for cancommand in cancommands:
		bytemessage = bytes.fromhex(cancommand.message)
		bytelength = bytes.fromhex('0' + str(cancommand.length))+b'\x00\x00\x00'
		byteid = bytes.fromhex(cancommand.message_id)
		if cancommand.is_extended_can and not (byteid[0] & 0x80) == 1:
			firstbyte = int(byteid.hex()[:2], 16)
			firstbyte += 0x80
			byteid = bytes.fromhex(hex(firstbyte)[2:] + byteid.hex()[2:])
		byteid = byteid[::-1]
		canstring = byteid + bytelength + bytemessage
		canstring = canstring.hex()
		canlist = []
		for i in range(0, len(canstring), 2):
			canlist.append(canstring[i:i+2])
		canoncelist.append([float(cancommand.delay), ' '.join(canlist), ['can' + str(cancommand.interface)]])
	for cangencommand in cangencommands:
		args = []
		args.append('can' + str(cangencommand.interface))
		args.append(cangencommand.gap)
		args.append(cangencommand.generate_extended_can)
		args.append(cangencommand.send_rtr_frame)
		if cangencommand.message_id and  len(cangencommand.message_id) > 0:
			args.append(cangencommand.message_id)
		else:
			args.append(None)
		if cangencommand.message_length and len(cangencommand.message_length) > 0:
			args.append(cangencommand.message_length)
		else:
			args.append(None)
		if cangencommand.can_data and len(cangencommand.can_data) > 0:
			args.append(cangencommand.can_data)
		else:
			args.append(None)
		args.append(cangencommand.number_of_can_frames_before_end)
		canutilslist.append([float(cangencommand.delay), "cangen", args])
	jsondict = {'Info': dict(ExperimentName=experiment.slugify_name(),Endtime=arrow.get(schedulinginfo.related_event.end).timestamp), 
	'Commands': dict(SSS_once=sssoncelist,SSS_loop=ssslooplist,CAN_once=canoncelist,CAN_utils=canutilslist,seed_key=seedkeylist)}
	pprint(jsondict)
	r = requests.post(ENGINEBEAGLEBONEIP, json=jsondict)
	if r.status_code == 200:

		#added TCP communications upon successful experiment scheduling
		for i in range(len(intfOrder)): #hosts tcp server for all CAN interfaces
			print("\nHosting TCP Server for CAN data at IP Address {} on Port {}".format(TCP_CONTROL_IP,canPorts[intfOrder[i]]))
			rxProcesses[i] = Process(target = rxServer, args = (intfOrder[i],))
			rxProcesses[i].start()

		ethCtrlData = b'\xFF\x00' #data to control server to tell all CAN interfaces (0xFF) to turn received CAN message streams on over TCP (0x00)
		#See documentation at https://github.com/Heavy-Vehicle-Networking-At-U-Tulsa/TruckCapeProjects/tree/master/SocketCAN
		tcpCtrlSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			tcpCtrlSocket.connect((TCP_CONTROL_IP, TCP_CONTROL_PORT))
		except OSError:
			print("Could not connect to TCP Control Server. Logging will not occur.")
		tcpCtrlSocket.send(ethCtrlData) #tells control server to open up TCP clients sending CAN data for interface can0 (port 2320) and can1 (port 2322)
		print("TCP rxon command sent to IP Address {} on Port {}".format(TCP_CONTROL_IP, TCP_CONTROL_PORT))

		run = models.RunResult.objects.get(experiment__exp_pk=experimentid,event=schedulinginfo.related_event)
		make_experiment_available(experimentid, run.pk, schedule=schedulinginfo.related_event.end + timedelta(seconds=15))
		notify.send(sender=experiment, actor=experiment.created_by, recipient=experiment.created_by, verb='has been started and will be available at ' + (schedulinginfo.related_event.end + timedelta(seconds=15)).strftime("%X %I:%M:%S %p"), level='info')
	
	else: 
		print('Non-200 status code on POST: ' + str(r.status_code))
		notify.send(sender=experiment, actor=experiment.created_by, recipient=experiment.created_by, verb='failed to start.  Please reschedule the experiment. (Error Code: ' + str(r.status_code) + ')', level='error')

@background(schedule=timezone.now())
def make_experiment_available(experimentid, runid):

	#added TCP control message to turn off tcpCAN streams
	ethCtrlData = b'\xFF\x01' #data to control server to tell all CAN interfaces (0xFF) to turn received CAN message streams off over TCP (0x01)
	#See documentation at https://github.com/Heavy-Vehicle-Networking-At-U-Tulsa/TruckCapeProjects/tree/master/SocketCAN
	tcpCtrlSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	try:
		tcpCtrlSocket.connect((TCP_CONTROL_IP, TCP_CONTROL_PORT))
	except OSError:
		print("Could not connect to TCP Control Server. Logging will not occur.")
	tcpCtrlSocket.send(ethCtrlData) #tells control server to open up TCP clients sending CAN data for interface can0 (port 2320) and can1 (port 2322)
	print("TCP rxoff command sent to IP Address {} on Port {}".format(TCP_CONTROL_IP, TCP_CONTROL_PORT))
	
	for i in range(len(intfOrder)):
		if rxProcesses[i]: rxProcesses[i].join() #wait for each process to finish
		#only if a process exists	

	print("Attempt merging temp files into one log file")

	experiment = models.Experiment.objects.get(exp_pk=experimentid)
	experiment.is_scheduled = False
	experiment.save()
	run = models.RunResult.objects.get(pk=runid)

	filename = experiment.slugify_name() + '_' + str(runid) + '_' + run.event.start.strftime("%c").replace(' ', '_') + ".csv"

	print("Merged Filename:", filename, "in directory", LOG_DIRECTORY)
	with open(LOG_DIRECTORY+filename, 'w') as file:
		tempFiles = [open(intf+"_temp.csv",'r') for intf in intfOrder] #list size of num of interfaces
		for i in range(len(intfOrder)): #for each interface's temp file
			for line in tempFiles[i]: #for each line in the temp file
				file.write(line) #write the line to the new file
	print("Completed file merging")

	quantities = models.ObservableQuantity.objects.filter(related_experiment__exp_pk=experimentid,related_run__pk=runid)
	url = ''
	success = True
	start_time = None
	if quantities.exists():
		success = False
	errorcode = None
	run.log = LOG_DIRECTORY+filename
	run.save()

	if success:
		notify.send(sender=experiment, actor=experiment.created_by, recipient=experiment.created_by, verb='is now available! Click "View Past Results" to view your results.', level='success')
		if hasattr(experiment, 'scheduling_info_for'):
			experiment.scheduling_info_for.delete()
	else:
		notify.send(sender=experiment, actor=experiment.created_by, recipient=experiment.created_by, verb='had an error with plot data retreival.  Please reschedule the experiment. (Error Code: ' + str(errorcode) + ')', level='error')

@background(schedule=timezone.now())
def send_complete_mail(experimentid, resultsurl):
	resultsurl = 'http://' + resultsurl
	experiment = models.Experiment.objects.get(exp_pk=experimentid)
	send_mail('Results Available For {title}'.format(title=experiment.experiment_title), expfinishedmessage.format(title=experiment.experiment_title,link=resultsurl), 'tu.tib.testbed@gmail.com', [experiment.created_by.email])
