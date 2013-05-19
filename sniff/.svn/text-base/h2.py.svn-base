#!/usr/bin/python -i
import serial
import Queue
import threading
import sys
from datetime import datetime
from db import db
from xbus import XbMsg, XbLocMsg
import xbus_types
import os
import time
dest = 2
us = 01
netid = 25
msgid = 0

ADC1 = 0xD
ADC2 = 0x10


def finddevice():
    devs = os.listdir('/dev')
    usbs = []
    for dev in devs:
        if dev.find('ttyUSB') == 0:
            usbs.append(dev)
    print "Found %d device candidates %s"%(len(usbs),usbs)
    usbs.sort()
    for dev in usbs:
        try:
            devpath = "/dev/%s"%dev
            ser = serial.Serial(devpath, 115200)
            print "opened device %s"%devpath
            return ser
        except:
            print "%s open failed"%devpath
    return None
            


class SerialTask(threading.Thread):
          def __init__(self):
              threading.Thread.__init__(self)
              self.txqueue = Queue.Queue()
              self.ser = finddevice()
              if not self.ser:
                  print "could not open any device"
                  sys.exit()
              #serial.Serial('/dev/ttyUSB0', 115200)

              self.settime()  # update embedded RTC
              self.db = db(debug=False)
          def local_packet(self,line):
              msg = XbLocMsg(line)
              #print "local_packet %s"%line
              if msg.error == False:
                  print msg
              #else:
              #    print "local packet error for line "+line
          def proc_input(self,line):
              bytes = line.split(' ')
              #print bytes

              if bytes[0] == 'FF':
                  self.local_packet(line)
                  return
              dest = int(bytes[1],16)
              net =  int(bytes[2],16)
              if net != 25:
                  print "wrong net %s"%line
                  return 0

              if len < 10:
                  print "wrong len %s"%line
                  return 0

              msg = XbMsg(line)
              if msg.error == False:
                  print msg
                  #print "msg in line %s"%line
                  #print "msg received type %s "%(hex(msg.hdr.type))
                  self.settime()
                  self.db.Insert(msg)

                  '''
                  try:
                      self.db.Insert(msg)
                  except:
                    print "DB error"
                  '''
                  
                  code = int(bytes[4],16)
                  src  = int(bytes[5],16)
                  if code == ADC2:
                      adc =  int(bytes[10],16)
                      adc += 256*int(bytes[11],16)
                      vcc = 2.0 * 2.5 * adc/4096.0
                  
                      print "supply reading for device %X  ADC %X vcc %.2f "%(src,adc,vcc)
                  return 1
              else:
                  print "XbMsg ERROR"
                  return -1


          def run(self):
              print "running stask"
              self.run = True
              while self.run:
                  #don't use full cpu!
                  time.sleep(0.05)
                  #read any lines available on rx
                  while self.ser.inWaiting() > 0:
                      line = self.ser.readline().lstrip()
                      self.proc_input(line)                                                         
                  #read any lines from from txqueue
                  while not self.txqueue.empty():                      
                      txline = self.txqueue.get()
                      self.ser.write(txline)            
                      self.txqueue.task_done()
                  #update embedded time
              print "Quitting rx task"
          def ucmd(self,addr,cmd):
              return "%02X%02X"%(addr,cmd)
          
          def settime(self):  # set time on hub device to system time
              timestr = self.ucmd(xbus_types.XB_HUB_ADDR,xbus_types.XB_PKT_SETTIME)             
              now = datetime.now()

              time = now.time()
              date = now.date()

              timestr += "%02X"%time.second
              timestr += "%02X"%time.minute
              timestr += "%02X"%time.hour
              timestr += "%02X"%date.day
              timestr += "%02X"%date.month
              timestr += "%02X"%(date.year-2000)
              timestr += "\0";
              
              self.txqueue.put(timestr)
                            
          def time(self,dev=xbus_types.XB_HUB_ADDR):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_GETTIME)
              self.txline(cmd+"\0")
          def stats(self,dev=xbus_types.XB_HUB_ADDR):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_GETSTATS)
              self.txline(cmd+"\0")
          def vcc(self,dev=xbus_types.XB_HUB_ADDR):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_SENDADC2)
              self.txline(cmd+"\0")
          def info(self,dev=xbus_types.XB_HUB_ADDR):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_INFO)
              self.txline(cmd+"\0")
          def wake(self,dev):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_STAYAWAKE)
              self.txline(cmd+"\0")
          def sleep(self,dev):
              cmd = self.ucmd(dev,xbus_types.XB_PKT_SLEEP)
              self.txline(cmd+"\0")              
          def txline(self,line):
              self.txqueue.put(line)
          def end(self):
              self.run = False

sert = SerialTask()
sert.start()

def end():
    sert.end()
    sys.exit(1)

def tm(dev=xbus_types.XB_HUB_ADDR):
    sert.time(dev)

def st(dev=xbus_types.XB_HUB_ADDR):
    sert.stats(dev)
def vcc(dev=xbus_types.XB_HUB_ADDR):
    sert.vcc(dev)

def info(dev=xbus_types.XB_HUB_ADDR):
    sert.info(dev)
    
def wake(dev):
    sert.wake(dev)
def sleep(dev):
    sert.sleep(dev)

print "commands coming"
info()
tm()
sert.info(dev=xbus_types.XB_HUB_ADDR)    
#ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=10)
'''
while 1:
    proc_cmd()
    line = ser.readline()
    print line,
'''
