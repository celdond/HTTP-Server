#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

ssize_t reader(int connection_port, char *buffer, ssize_t size);

#endif
