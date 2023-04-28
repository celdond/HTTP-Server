# HTTP Server

A client-server model utilizing HTTP protocols to communicate file transfer.

## Installation

All code was written in C in Ubuntu version 20.04.5 using its unmodified distribution of C.
Additional details are as described in the corresponding Makefile.

In Ubuntu, the repository should be able to run out of the box.
Simply clone, and then use make to run the corresponding file.

## Use

To use the server, an access via client is necessary.  In this repository, the server is located in
the server file.  Bundled together is a client, in the corresponding client file, which provides 
access to the server via TCP sockets.

Use of the client is not explicitly neccessary, however, and any such client, like curl, can be used
to prompt the server while running.

### Server

To build the server:

```bash
make
```
or
```bash
make all
```
To run the server:
```bash
./httpserver [port] -N [thread count]
```
port: the port number the server will be listening to.
thread count: the number of worker threads the server will use.

### Client

To build the client:

```bash
make
```
or
```bash
make all
```

To run the client:
```bash
./client [port] -N [thread count]
```
port: the port number the client will send requests to.
thread count: the number of worker threads the client will use to send requests.

## Functionality

### Server

The server uses HTTP methods as an interface to provide file transfer.  Server functionality is tied
specifically to HTTP requests, and it will wait until it has received such a request to operate.

#### Methods

##### HEAD

##### GET

##### DELETE

##### PUT
