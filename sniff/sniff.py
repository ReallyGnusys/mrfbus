#!/usr/bin/python

import serial



#ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=10)
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=10)

while 1:
    line = ser.readline()
    print line,
