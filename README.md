# embedded-homework

This is an implementation of the "embedded-homework" assignment that Density provided here:

https://github.com/DensityCo/embedded-homework

Please see the README for the requirements and details.

## Design Process

I started with an existing implementation of a daemon that echoed back messages from multiple clients in a single thread. It was written in C in single source file. When sockets are involved (or anything POSIX related), I like having a working example to reference because it's much easier than just digging through man pages.

My code is entirely mine, and written in C++ with C calls when needed.

### count (cpp)
* Contains main() and an infinite loop that blocks on a call to select() after initializing a file descriptor set. select() will return if there is any data available on the file descriptors. Initially, this set only contains the master socket. Additional sockets are created upon connection from clients.

### command (hpp/cpp)
* Contains the command string parser. The parseCommand() function is a static factory that returns a unique_ptr to a descendant of the base class Command. The Command class implements a simple command pattern with the virtual function execute(), which is overriden by each subclass to perform whatever task the command needs.

### socket (hpp)
* Contains code for three classes: Socket, MasterSocket, and ClientSocketSet.
  * Socket is a simple base class that wraps a file descriptor (int) used by the client sockets. Helper functions are provided to send and receive data.
  * MasterSocket contains code to init an IPV4 TCP socket and listen for connections on a single port. It inherits from Socket.
  * ClientSocketSet wraps a simple list of Socket instances, one for each client connection. It provides helper functions for broadcasting data to all clients, closing the client connections, and adding clients to file descriptor sets. This class would be much more efficient if I had used an ordered map to store the Sockets. Removing a disconnected client would go from O(n) to constant time, along with updating the max file descriptor.

### 

## Build

```
mkdir build
cd build
cmake ..
make
```

## Run/Test

### Server

```
./count
```

### Client

```
telnet localhost 8089
INCR 50
DECR 25
OUTPUT
```

## Systemd Requirements

### Restart After Crash
https://ma.ttias.be/auto-restart-crashed-service-systemd/

You can add the Restart option to the [Service] stanza of the service file.

```
$ cat /etc/systemd/system/count.service
[Unit]
Description=Count Daemon
After=network-online.target
Wants=network-online.target systemd-networkd-wait-online.service

StartLimitIntervalSec=500
StartLimitBurst=5

[Service]
Restart=on-failure
RestartSec=5s

ExecStart=/path/to/count

[Install]
WantedBy=multi-user.target
```

The above will react to anything that stops your daemon: a code exception, someone that does kill -9 <pid>, etc. As soon as your daemon stops, systemd will restart it in 5 seconds.

In this example, there are also StartLimitIntervalSec and StartLimitBurst directives in the [Unit] section. This prevents a failing service from being restarted every 5 seconds. This will give it 5 attempts, if it still fails, systemd will stop trying to start the service.

### STDERR To Journal
https://www.freedesktop.org/software/systemd/man/systemd.exec.html

Make sure the following lines are in you service file:
```
StandardOutput=journal
StandardError=journal
```

## Reference
https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/?ref=rp
