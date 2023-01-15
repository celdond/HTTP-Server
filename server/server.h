#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

ssize_t reader(int connection_port, char *buffer, ssize_t size);

void head(int connfd, char *buffer);

void send_message(int connfd, int mess_num, char *message, ssize_t size);

#endif
