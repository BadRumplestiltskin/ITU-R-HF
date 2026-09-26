#### Introduction
This directory contains the top-level Makefile for building ITURHFProp and the associated libp533.so and libp372.so libraries on Linux systems.  The Makefile recognises the targets all, clean and install.
#### Building and Installing the Application
The application is built using the following command;
```
$ make all
```
The following command installs the libraries and application in /usr/local/lib and /usr/local/bin ;
```
$ sudo make install
```
The programs are linked against libp533.so and libp372.so, and look for them in the lib directory beside their bin directory (an rpath of $ORIGIN/../lib), so an installed copy runs without ldconfig or LD_LIBRARY_PATH, under any prefix. To run the programs from the build tree instead, name the two build directories on the library path:
```
$ LD_LIBRARY_PATH=../P533/Linux:../P372/Linux ../ITURHFProp/Linux/ITURHFProp -v
```
On macOS use DYLD_LIBRARY_PATH, set on the command itself (as above) rather than exported: macOS removes DYLD_ variables when it starts a system program such as /bin/sh.

#### Data Directory
Operation of the application requires the presence a number of data files. This may be performed manually using a location of the user's choice or by using the command ```sudo make install-data``` to copy the files to /usr/local/share/p533/data.  The location of the data directory is a required parameter in input files.  e.g. If the files are copied to /usr/local/share/p533/data, input files should contain the line;
```
DataFilePath "/usr/local/share/p533/data/"
```
The following files should be copied from the P372/Data directory to the nominated data directory on the host system. 

COEFF01W.txt
COEFF02W.txt
COEFF03W.txt
COEFF04W.txt
COEFF05W.txt
COEFF06W.txt
COEFF07W.txt
COEFF08W.txt
COEFF09W.txt
COEFF10W.txt
COEFF11W.txt
COEFF12W.txt
ionos01.bin
ionos02.bin
ionos03.bin
ionos04.bin
ionos05.bin
ionos06.bin
ionos07.bin
ionos08.bin
ionos09.bin
ionos10.bin
ionos11.bin
ionos12.bin
P1239-3 Decile Factors.txt
