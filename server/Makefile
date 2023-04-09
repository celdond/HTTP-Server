CFLAGS = -Wall -Wextra -pedantic -Wshadow -pthread -O2 -g
CC = clang $(CFLAGS)

all		:httpserver

httpserver	:httpserver.o server.o drone.o
	$(CC) -o httpserver httpserver.o server.o drone.o

drone.o		:drone.c drone.h
	$(CC) -c drone.c drone.h

server.o	:server.c server.h
	$(CC) -c server.c server.h

httpserver.o    :httpserver.c
	$(CC) -c httpserver.c

clean:
	rm -rf httpserver httpserver.o server.o drone.o server.h.gch drone.h.gch
