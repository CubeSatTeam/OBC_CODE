#!/bin/python3

import socket
import time

sock= socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
sock.connect("/tmp/telegraf.sock")

currt=time.time_ns()

dataFormat="testData,tag1=2 data={0} {1}\n"

sentstring=dataFormat.format(0,currt)
print(sentstring,sep="")
sock.send(sentstring.encode('utf8'))

sentstring=dataFormat.format(1,currt+1000000000)
print(sentstring,sep="")
sock.send(sentstring.encode('utf8'))

sentstring=dataFormat.format(1.5,currt+2000000000)
print(sentstring,sep="")
sock.send(sentstring.encode('utf8'))

sentstring=dataFormat.format(2.5,currt+3000000000)
print(sentstring,sep="")
sock.send(sentstring.encode('utf8'))

sock.close()
