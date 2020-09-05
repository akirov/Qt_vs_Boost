Client-Server with QtNetwork, Boost.ASIO and C++11/POSIX
========================================================

These are several C++ implementations of a simple client-server protocol to
demonstrate and compare how to use QtNetwork and Boost.ASIO for asynchronous
networking and multithreading. C++11/POSIX implementation is on the way.

The server maintains multiple "streaming" channels, generating random numbers
in regular time intervals, and sends these streams to connecting clients upon
client request. Data is served in format "ChannelNo, Value". It can be sent as
ASCII (easiest), binary (most efficient), or JSON (most maintainable). On the
server each channel ticks in its own thread.

The client control connection is telnet-like. After successfully connecting,
the server prompts for a command. Three commands are accepted - "start",
"stop" and "quit". The "start" command tells the server to start sending the
streams (the random number sequences). The "stop" command stops the acquisition.
Finally, "quit" disconnects the client from the server and exits the client.

Several possible architectures:
- On the server there can be a TCP connection and a thread per client, where
control commands are received and streams data is sent. Doesn't scale well.
- Or, there can be a TCP control connection (clients-to-server) for client
commands (this is a telnet server basically), and UDP data channels
(server-to-clients) with receiving sockets on the clients side. UDP data
channels can be per stream and we can send data in stream producer's context,
looping over the clients, or we can have a dedicated data sending thread with a
queue (ring buffer) filled by data producer threads and emptied when data is
sent (or the buffer is full). Since we are sending random numbers we don't care
about re-ordering and packet drops (which can happen with UDP), otherwise we
will need sequence numbers or a TCP channel for data. On the client side we can
have one control thread and one data thread.
- We can even have a UDP multicast group per server stream, and the clients
can join and leave this group.

In any case we need buffering for control commands on the server and for the
data on the clients.
