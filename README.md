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

All files for the server are located in the files folder

#### Methods

##### HEAD

```bash
HEAD /[filename] HTTP/1.1
```

Returns the length of the specified file in the server.

##### GET

```bash
GET /[filename] HTTP/1.1
```

Returns the specified file in the server.

##### DELETE

```bash
DELETE /[filename] HTTP/1.1
```

Deletes the specified file from the server.

##### PUT

```bash
PUT /[filename] HTTP/1.1
Content-Length: [file-length]
```

Places the specified file from the client to the server.

### Client

While unnecessary for use of the server, bundled in the repository is a client server specifically created 
to request file transfer with a HTTP server.

#### Request Creation

The client manages request by reading text files in the requests folder.

Request files follow a format similar to csv files, where each entry is on 
a new line with the required information being separated by a comma.

```bash
[METHOD],[FILE_NAME]
[METHOD],[FILE_NAME]

```

Example:

```bash
GET,Example.txt

```
