#!/bin/python3

import time
import socket
import sys

cmdArgs=sys.argv

if len(cmdArgs)<2:
	print("You need to specify arguments:")
	print("<cmdCode> [<arg1> [<arg2> ...")
	sys.exit()

cmdString=""
for arg in cmdArgs[1:]:
	cmdString+=arg
	cmdString+=" "

sockName="/tmp/CDH.sock"
timeout=1	

client=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
client.bind("")
client.settimeout(timeout)

try:
	client.sendto(bytearray(cmdString,"utf-8"),sockName)
except:
	print("ERROR: failed to send data to server (perhaps service not running?)")
else:
	try:
		data,addr=client.recvfrom(4096)
		print(data.decode("utf-8"),"")
	except:
		print("ERROR: Timeout reached, no response from server")

client.close()
