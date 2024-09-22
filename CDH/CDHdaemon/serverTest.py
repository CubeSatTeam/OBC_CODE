#!/bin/python3

import time
import socket
import os, os.path

sockName="/tmp/CDH.sock"

if os.path.exists(sockName):
	os.remove(sockName)
	
server=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM)
server.bind(sockName)
#server.setblocking(False)

while 1:
	data,addr=server.recvfrom(4096)
	strstr=data.decode("utf-8").split()
	print("recvfrom: {0} {1}".format(strstr,addr))
	server.sendto(b"Response",addr)
	
os.unlink(sockName)

