#!/bin/python3

import smbus2
import time

address=0x48
command=0x8C

print("ADC tester (ADS7828E)")

#creating SMBus instance on I2C bus 1
bus=smbus2.SMBus(1)

#turning on internal reference
bus.write_byte(address, command)

#waiting 5 milliseconds to stabilize Vref
time.sleep(0.005)

#requesting conversions
convres=[]
for ch in range(8):
	convres.append(bus.read_i2c_block_data(address,command+0x10*ch,2))

#printing conversion results from the list of read messages
print("Conversion results:")
for ch in range(len(convres)):
	print("Channel {0}: {1}".format(ch,int(convres[ch][0]*256+convres[ch][1])))

