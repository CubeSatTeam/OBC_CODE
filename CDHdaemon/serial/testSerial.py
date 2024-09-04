#!/bin/python3

#this test script will be used to test the whole serial
#communication structure of the raspberry

#to do so, it uses the loopback serial line which was
#defined in serialInterface.c library, sending every
#message defined in messages.py to itself and checking
#that it receives them

from ctypes import *
import struct
import sys
import time

print("Testing serial loopback line")

serial = CDLL("./serialInterface.so")
print("Maximum serial payload length: {0}\n".format(serial.getMaxLen()))
serial.initUART(0,0)
print("")

testPass=True

for i in range(10):
	msg=[]
	for y in range(10):
		msg.append(i*10+y)
	
	bufftx=bytes(msg)
	print("Created message: {0}".format(bufftx.hex()))

	print("Sending message to serial line")
	serial.sendUART(bufftx,len(bufftx),0)

	buffrx=bytes(serial.getMaxLen())
	
	#waiting some time to allow messages to arrive
	time.sleep(0.01)
	
	print("Trying to receive from serial line")
	l=serial.receiveUART(buffrx,len(buffrx))
	print("Received {0} bytes".format(l))

	if buffrx[:l]==bufftx:
		print("Message correctly delivered!\n")
	else:
		print("ERROR: message wasn't correcly delivered\n")
		testPass=False
		
if testPass:
	print("\nTEST SUCCESSFULL.")
else:
	print("\nTEST FAILED.")
	
serial.deinitUART()
