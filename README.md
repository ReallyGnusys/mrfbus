**mrfbus** is a framework for building ultra low-power microcontroller network applications

[![Build Status](https://travis-ci.org/ReallyGnusys/mrfbus.svg?branch=master)](https://travis-ci.org/ReallyGnusys/mrfbus)

## FAQ

### Is it ready?

Probably not. 

We moved it to github to allow travis integration on 24/3/2016.
All claims concerning mrfbus capabilities will be made by travis.
 
Check logs for details.
    
### What's the point of it?

To simplify deployment of ultra low power microcontroller applications networked with linux powered hosts.
    
### What micros are currently supported?

[TI CC430F5137](http://www.ti.com/product/CC430F5137)

### Why not use contiki? ###

There appears no support for CC430F5137. 

Contiki is a much more fully featured OS with IP network and filesystem support. MRFBUS is intended to connect ultra lightweight sensor and actuator devices to a linux host with minimum fuss, rather than make the devices directly accessible to the wider IP network. MRFBUS provides it's own network to handle delivery of messages across multiple hops as well as an execution framework to support development of low power, high reliability motes.

The default MRFBUS app compiles into less than 16K of ROM for a CC430F5137.

### What does the default MRFBUS app do?

Implements the core MRFBUS command set.

The core MRFBUS command set supports network mapping and debug by higher level applications. 
It allows an application to determine essential information about a mote and it's functions.
Provides network settime and gettime implementations for clock synchronisation.
Each MRFBUS device participates in message routing over the network by default. All devices can route messages between interfaces.


### How does it interface with linux?

Linux is a supported architecture for MRFBUS.

Mrfbus can be run and tested on linux and linux hosts can participate in mrfbus networks, alongside microcontrollers.

A common use-case is for a linux host (e.g. raspberry PI ) to act as network root/hub/bridge via an appropriate mrfbus application.

### What does it provide?

All code (libraries, makefiles, linker-scripts ) to build OS-less micro-controller apps with out of the box network functionality making fair use of  low power architecture features. In addition, a single threaded linux implementation of mrfbus is provided.

Core command set implemented by all devices.

### What's the address space?

8 bits.  Address 255 is broadcast.

### What interfaces are supported?
    
Sub-GHZ RF (using cc430f5137 packet radio ) uart and unix fifo.

