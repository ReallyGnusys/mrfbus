**mrfbus** is a framework for building ultra low-power microcontroller network applications in a linux environment

[![Build Status](https://travis-ci.org/ReallyGnusys/mrfbus.svg?branch=master)](https://travis-ci.org/ReallyGnusys/mrfbus)

## Quickstart instructions
 * Ensure you're running a credible linux distro and looking at a bash shell prompt
 * Create a directory to clone MRFBUS project into, and set env variable MRFBUS_HOME to this value
    ```
      export MRFBUS_HOME='path to mrfbus clone directory'
      mkdir -p $MRFBUS_HOME
     
     
 * cd to MRFBUS_HOME and clone repo
    ```
    cd $MRFBUS_HOME
    git clone https://github.com/ReallyGnusys/mrfbus.git .
    
    
    
 * Create a directory to build and run your MRFBUS project and cd to it
   ```
    export MRFBUS_RUN=~/mrfrun
    cd $MRFBUS_RUN
    
 * Setup example linux test project
    ```
     $MRFBUS_HOME/mrf_proj setup test
     ```
 * Build example test project
    ```
     $MRFBUS_HOME/mrf_proj build
     ```

 * Run example test project
    ```
     $MRFBUS_HOME/mrf_proj run
   ```

 

## FAQ

    
### What's the point of it?

To simplify deployment of ultra low power microcontroller applications networked with linux powered hosts.
    
### What micros are currently supported?

[TI CC430F5137](http://www.ti.com/product/CC430F5137)


### What does it provide?

All code (libraries, makefiles, linker-scripts ) to build OS-less micro-controller apps with out of the box network functionality making fair use of  low power architecture features. In addition, a single threaded linux implementation of mrfbus is provided.

Core command set implemented by all devices.


### What does the default MRFBUS app do?

The core MRFBUS command set supports network mapping and debug by higher level applications. 
It allows an application to determine essential information about a mote and it's functions.
Provides network settime and gettime implementations for clock synchronisation.
Each MRFBUS device participates in message routing over the network by default. All devices can route messages between interfaces.

The default MRFBUS app compiles into less than 16K of ROM for a CC430F5137.


### How does it interface with linux?

Linux is a supported architecture for MRFBUS. Any modern linux system should be able to build and run MRFBUS.

MRFBUS can be run and tested on linux and linux hosts can participate in mrfbus networks, alongside microcontrollers.


### What's the device address space?

8 bits.  Address 255 is reserved for broadcast.

### What interfaces are supported?
    
Sub-GHZ RF (using cc430f5137 packet radio ) uart and unix fifo.


### Yes but how could I control and monitor my home using MRFBUS? ###

The project includes an example MRFBUS server written in python in /land which is used for testing by travis,  but is intended to form the basis of an application server in due course.




