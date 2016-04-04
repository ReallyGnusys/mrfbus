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

### Why is it better than something I can lash up from a blinking LED example program in a weekend? ###

MRFBUS provides a multi-hop message delivery service with automatic retries spanning RF, uart and unix fifo channels.  All MRFBUS devices are capable of routing packets across interfaces, so mixed microcontroller networks using UART and RF segments can be quickly deployed. 

Some effort has been made to separate the system code from application code as well as to organise h/w dependent code in a consistent way. 

Linux devices can participate in MRFBUS networks, enabling higher level applications to access MRFBUS devices from the wider network.

The linux implementation of MRFBUS uses epoll to emulate the interrupt driven microcontroller implementation using a single thread that uses minimal CPU time.  Most code is common between architectures,  allowing core development and testing to be largely carried out in a linux environment. 

### What does the default MRFBUS app do?

Implements the core MRFBUS command set.

The core MRFBUS command set supports network mapping and debug by higher level applications. 
It allows an application to determine essential information about a mote and it's functions.
Provides network settime and gettime implementations for clock synchronisation.
Each MRFBUS device participates in message routing over the network by default. All devices can route messages between interfaces.

The default MRFBUS app compiles into less than 16K of ROM for a CC430F5137.


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

