
To compile the source you need qmake, which is part of qt-devel package
(or Qt SDK on Windows) and g++ (MinGW on Windows).
On Linux, if qmake is not found you may need to specify QTDIR var
and add $QTDIR/bin (containing qmake) to the path like this:

QTDIR="/usr/lib/qt4"
PATH=$QTDIR/bin:${PATH}
export QTDIR PATH

After making sure that 'qmake' is present, just run 'make' in project's
root directory (containing client and server subdirs). The executables
will be in ./exe dir.

Command line options (port, number of channels, and time interval for
generating the random numbers):
./server p=1234 ch=3 t=5
Defaults are: port=4321, channels=2, t=1 sec.
The client needs server host (mandatory) and port (optional, defaults
to 4321):
./client s=localhost p=1234


Example usage:

[asen@localhost exe]$ ./server
Main server thread:  3079334240 
Created  2  channels with timers. 
Listening on port  4321 
12  Connecting  
Starting acquisition for client  12 
 Client  12  Disconnected 
Removing client  12 


[asen@localhost exe]$ ./client s=localhost
CLIENT> Connecting... 
Connected! 

CLIENT> start
CLIENT> Received:  " 2:6 1:9" 
Received:  " 2:3" 
Received:  " 1:7" 
Received:  " 2:4 1:5" 
Received:  " 2:2 1:5" 
Received:  " 2:4 1:7" 
Received:  " 2:4 1:4" 
Received:  " 2:3 1:0" 
Received:  " 2:7 1:8" 
Received:  " 2:6 1:8" 
Received:  " 2:8 1:4" 
Received:  " 2:3 1:1" 
Received:  " 2:4 1:9" 

CLIENT> Received:  " 2:2 1:0" 

CLIENT> stop
Received:  " 2:9 1:2" 

CLIENT> 
CLIENT> 
CLIENT> quit
Disconnected! 
[asen@localhost exe]$
