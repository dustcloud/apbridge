# AP Bridge 

This repository contains the AP Bridge software (sometimes referred to as APC), which provides a connection between the SmartMesh IP VManager and the AP Mote. 

## Contents

The AP Bridge sources are separated into several directories:

- APInterface - VManager-APC protocol and communication 
- common - common utility functions
- logging - logging implementation based on log4cxx
- pkg - files used for building a distribution package, currently targeted at .deb file for Ubuntu or Raspian
- python - APC Console for command line interaction with AP Bridge
- rpc - Inter-process communication with APC Console
- scripts - build scripts for managing the internal release process
- shared - common header files defining the protocol with the AP Mote
- site_scons - build scripts for code generation
- watchdog - internal software watchdog

## Build instructions

The AP Bridge software requires several external libraries that must be available on the build host. SCons is used to build the AP Bridge.

```
$ scons apbridge target=i686-linux
$ scons apbridge_pkg target=i686-linux
```
