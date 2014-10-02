Qt_vs_Boost - Client-Server with QtNetwork and Boost.ASIO
=========================================================

These are two C++ implementations of a simple client-server protocol to
demonstrate and compare how to use QtNetwork and Boost.ASIO for asynchronous
networking and multithreading.

The server maintains two "streaming" channels, generating random numbers in
regular time intervals, and sends these streams to connecting clients upon
request.

The client is telnet-like. After successful connection to the server it
prompts for a command. Three commands are accepted - "start", "stop" and
"quit". The "start" command tells the server to start sending the streams
(the random number sequences). The "stop" command stops the acquisition.
Finally, "quit" exits from the client.

On the server each channel ticks in its own thread. There is also a thread
per client conneection to manage network send and receive operations and
interpret commands. All communication is asynchronous, based on events and
handlers.

ToDo:
- buffering
