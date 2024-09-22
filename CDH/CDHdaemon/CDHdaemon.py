#!/bin/python3

import smbus2
import time
import threading
import queue
import sys
import socket
import ctypes
import os
import signal

#set stdout in line buffering mode
sys.stdout.reconfigure(line_buffering=True)

sys.path.append("./messages")
import messages as msg
serial = ctypes.CDLL("./serial/serialInterface.so")
print("Maximum serial payload length: {0}\n".format(serial.getMaxLen()))

#ADC thread ---------------------------
address=0x48
command=0x8C
#creating SMBus instance on I2C bus 1
bus=smbus2.SMBus(1)
ADCperiod=5 #sampling period in seconds
#--------------------------------------

#Client thread ------------------------
cdhSockPath="/tmp/CDH.sock"
#clientQueueTx=queue.Queue() #queue to send data to client
#clientQueueTxTimeout=0.1 #timeout for reading from client tx queue
clientQueueRx=queue.Queue() #queue to receive data from client
uartTimeout=0.010 # timeout for uart transmission with ack
uartRetries=2 #number of retries in case of failed ack (total 3 tries)
#--------------------------------------

#CDH thread ---------------------------
availableCommands=[0,1] #command codes available from client
clientQueueRxTimeout=0.002 #timeout for reaing from client rx queue
#--------------------------------------

#Logging thread -----------------------
logQueue=queue.Queue() #queue to send strings for telegraf/log file
logQueueTimeout=0.05 #timeout for log queue read (to reduce CPU starving)
enableFileLog=True  #file logging enabled/disabled
logFilePath="telegrafLog.txt" #log file path
fileBuffering=2048 #file buffer size (see python file buffering modes for details)
		#file buffer has been set to a big value because a lot of I/O from 
		#ADCS is expected, in case of service interruption this amount
		#of data can be lost

fileRetryTime=3 #time waited after log file opening failure before retrying
#--------------------------------------

stopThreads=threading.Event() #thread safe flag to signal to all threads to stop
threadTermTimeout=3 #timeout for thread join() after termination

#setting up the ADC
def setupADC(printerr=True):
	try:
		#turning on internal reference
		bus.write_byte(address, command)
	except:
		if printerr:
			print("ERROR: Failed to set up ADC")
		
	#waiting to stabilize Vref
	time.sleep(0.005)

#reading the ADC
def readADC(printerr=True):
	#requesting conversions
	convres=[0 for _ in range(8)]
	try:
		for ch in range(8):
			convOut=bus.read_i2c_block_data(address,command+0x10*ch,2)
			convres[ch]=convOut[0]*256+convOut[1]
	except:
		if printerr:
			print("ERROR: Failed to read the ADC, trying to set it up again")
		setupADC(printerr)
		
	return convres
	
def adcThread():
	print("ADC thread started")
	
	global logQueue
	global address
	global command
	global bus
	global ADCperiod
	global stopThreads
	
	print("Setting up ADC")
	setupADC()
	while 1: #thread loop
		if stopThreads.is_set(): #need to close thread
			break
		#lightweight method to get periodic task without strict control on period overflow or system time changes
		time.sleep(ADCperiod-time.time()%ADCperiod)		
		#getting ADC data
		#(for now error print in case of failed read is disabled to not fill the log
		#if you want to enable error print every time pass True to the function)
		ADCdata=readADC(False)
		#measurements reconstruction
		for ch in range(8):
			ADCdata[ch]=ADCdata[ch]*2.5/4095 #reconstructing measured voltage
		#reconstructing measurements
			ADCdata[0]=ADCdata[0]*2 	#V5
			ADCdata[1]=ADCdata[1]*3.326667 	#I5
			ADCdata[2]=ADCdata[2]*5.255319 	#VB
			ADCdata[3]=ADCdata[3]*3.326667 	#IB
			
		#writing data on telegraf/file
		strFormat="housekeepingOBC,source={0} VB={1},IB={2},V5={3},I5={4} {5}\n"
		finalString=strFormat.format("OBC",ADCdata[2],ADCdata[3],ADCdata[0],ADCdata[1],time.time_ns())
		#print(finalString,sep="")
		#sending data to logThread
		logQueue.put(finalString)

'''
def clientThread():
	print("Client thread started")
	
	global cdhSockPath
	global clientQueueTx
	global clientQueueRx
	global stopThreads
	global clientQueueTxTimeout
	
	server=None
	#creating socket for client
	print("Creating client socket")
	try:
		if os.path.exists(cdhSockPath):
			os.remove(cdhSockPath)
	
		server=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
		server.bind(cdhSockPath)
		server.setblocking(False)
	except:
		print("ERROR: Failed to create client socket {0}".format(cdhSockPath))
	
	addr=None
	
	while 1:
		if stopThreads.is_set(): #need to close thread
			break
			
		#try receiving data from client socket
		try:
			datain,addr=server.recvfrom(4096)
		except BlockingIOError:
			pass #if timeout reached don't do anything
		except: #other exceptions
			print("ERROR: Failed to read from client socket, trying to recreate socket")
			try:
				if os.path.exists(cdhSockPath):
					os.remove(cdhSockPath)
			
				server=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
				server.bind(cdhSockPath)
				server.setblocking(False)
			except:
				print("ERROR: Failed to create client socket {0}".format(cdhSockPath))
		else:
			clientQueueRx.put(datain.decode("utf-8"))
			
		#see if there's some output for client
		try:
			dataout=clientQueueTx.get(timeout=clientQueueTxTimeout)
		except: #nothing to get
			pass
		else:
			try:
				server.sendto(dataout.encode("utf-8"),addr)
			except: #in case client was closed or other errors, just ignore the output
				pass

	print("Closing and deleting client socket")
	try:
		server.close()
	except:
		pass
	
	try:
		os.remove(cdhSockPath)
	except:
		pass	
'''
		
def logThread():
	print("Log thread started")
	
	global logQueue
	global logQueueTimeout
	global logFilePath
	global enableFileLog
	global fileBuffering
	global fileRetryTime
	global stopThreads
	
	socketState=0
	
	fileTryTime=0
	fileState=0
	
	logFile=None
	
	while 1: #thread loop
		if stopThreads.is_set(): #need to close thread
			break	
		
		#checking if log file is not opened
		if enableFileLog and fileState==0 and (time.time()-fileTryTime)>fileRetryTime:
			fileTryTime=time.time()
			try:
				logFile=open(logFilePath,"w",fileBuffering) #opening log file
			except:
				print("ERROR: Failed to open log file ({0}), retrying in {1} seconds".format(logFilePath,fileRetryTime))
			else:
				fileState=1
				print("log file ({0}) opened".format(logFilePath))
				
		
		#checking if there's some data to be logged
		try:
			log=logQueue.get(timeout=logQueueTimeout)
		except:
			pass
		else:		
			if enableFileLog and fileState==1:
				try:
					logFile.write(log)
				except:
					print("ERROR: Failed to write data on file")
					logFile.close()
					fileState=0
	
	if enableFileLog:
		print("Closing log file")
		logFile.close()
		
def cdhThread():
	print("CDH thread started")
	
	global logQueue
	#global clientQueueTx
	global clientQueueRx
	global stopThreads
	global clientQueueRxTimeout
	global uartTimeout
	global uartRetries

	#initializing serial line towards ADCS
	print("Initializing UART")
	serial.initUART(ctypes.c_float(uartTimeout),ctypes.c_uint8(uartRetries))
	
	while 1: #thread loop
		if stopThreads.is_set(): #need to close thread
			break
	
		'''
		#try receiving data from client queue
		try:
			data=clientQueueRx.get(timeout=clientQueueRxTimeout)
			print("Client DATA: \n ", data)
		except Exception as e:
			#print("Didn't receive from queue, ERROR MESSAGE: \n ", e)
			pass
		else: #something received
			print("RECEIVED client DATA: \n",data)
			if data.split(maxsplit=1)[0]=="help":
				helpstring='Available commands (array elements should be passed inside quotes " "):\n'
				for available in availableCommands:
					try:
						helpstring+="{0}\n\n".format(msg.msgDict[available]())
					except:
						pass
						
				clientQueueTx.put(helpstring)
			
			else:
				#Here we handle all the possible commands from client
				try:
					#extract message struct from command string
					msgStruct=msg.parseStruct(data)
					if msgStruct.code not in availableCommands:
						raise Exception
				except:
					clientQueueTx.put("ERROR: the requested command was not recognized or the arguments format is wrong\nYou can list avilable commands with 'help'\n")
				else:
					#send message over serial
					bufftx=bytes(msgStruct)
					retVal=serial.sendUART(bufftx,len(bufftx),1) #requesting also an ack from ADCS
					if retVal:
						clientQueueTx.put("{0} message sent\n".format(data.split(maxsplit=1)[0]))
					else:
						clientQueueTx.put("ERROR, ADCS didn't acknowledge {0}\n".format(data.split(maxsplit=1)[0]))
		'''
						
		#try reading message from serial
		buffrx=bytes(serial.getMaxLen())
		l=serial.receiveUART(buffrx,len(buffrx))
		#print("l: ", l)
		if l != 0:
			#check message code
			code=buffrx[0]

			# keep only codes 21 and 22
			if code==21:
				#if the code and the length correspond to a valid message
				if code in msg.msgDict.keys() and ctypes.sizeof(msg.msgDict[code]) == l:
					# ------ HERE WE HANDLE EACH MESSAGE CODE FROM ADCS -------			
					match msg.msgDict[code].__name__:
						case "attitudeADCS" | "housekeepingADCS" | "opmodeADCS": #attitude telemetry message
							#saving current timestamp
							currt=time.time_ns()
							
							#creating the corresponding message struct
							newstruct=msg.msgDict[code].from_buffer_copy(buffrx[:l])
							
							#building influxdb write string
							influxstr=msg.msgDict[code].__name__+"," #inserting message name as dataset name
							influxstr+="source=ADCS "#inserting source tag
							#iteratively inserting all fields
							for f in newstruct._fields_:
								#checking if value is an array
								if isinstance(getattr(newstruct,f[0]),ctypes.Array):
									arraylist=getattr(newstruct,f[0])[:]
									for index in range(len(arraylist)):
										influxstr+="{0}={1}".format("{0}[{1}]".format(f[0],index),arraylist[index])
										if (index+1)!=len(arraylist):
											influxstr+=","							
								else:
									influxstr+="{0}={1}".format(f[0],getattr(newstruct,f[0]))
								if f!=newstruct._fields_[-1]:
									influxstr+=","
							
							#appending timestamp
							influxstr+=" {0}\n".format(currt)
							print(f"\n Data received !")
							#sending to telegraf queue
							logQueue.put(influxstr)
							print(f"\n Data sent to logQueue...")
							
						case _: #default case
							print("WARNING: {0} message from ADCS not handled".format(msg.msgDict[code].__name__))
				else:
					print("WARNING: Received unknown message from ADCS (code {0} length {1})".format(code, l))

			else:
				pass
	
	print("Closing UART")	
	serial.deinitUART()


#running all threads
print("Starting threads")
adcT=threading.Thread(target=adcThread, daemon=True)
#cliT=threading.Thread(target=clientThread, daemon=True)	
cdhT=threading.Thread(target=cdhThread, daemon=True)	
logT=threading.Thread(target=logThread, daemon=True)
	
adcT.start()
#cliT.start()
cdhT.start()
logT.start()

print("All threads started")

def stop_handler(sig, frame): #handler function for stop signals
	global stopThreads
	global adcT
	#global cliT
	global cdhT
	global logT
	global threadTermTimeout

	stopThreads.set() #stopping all threads
	
	print("Received termination signal")
	
	#waiting for all threads to join
	adcT.join(timeout=threadTermTimeout)
	#cliT.join(timeout=threadTermTimeout)
	#cdhT.join(timeout=threadTermTimeout)
	logT.join(timeout=threadTermTimeout)
	
	print("All threads terminated or timed out, BYE!")
	sys.exit()
	
#setting signal handler
signal.signal(signal.SIGTERM, stop_handler)
signal.signal(signal.SIGINT, stop_handler)

while 1:
	#checking if all threads are still alive
	allAlive=True
	if not adcT.is_alive():
		allAlive=False
	#if not cliT.is_alive():
		#allAlive=False
	if not cdhT.is_alive():
		allAlive=False
	if not logT.is_alive():
		allAlive=False
		
	if not allAlive:
		print("A thread unexpectedly closed, terminating execution")
		os.kill(os.getpid(),signal.SIGTERM)
	
	time.sleep(1)
			
