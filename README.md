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

### Client

To build the client:

```bash
make
```
or
```bash
make all
```
