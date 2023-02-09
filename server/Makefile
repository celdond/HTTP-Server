CFLAGS = -Wall -Wextra -pedantic -Wshadow -pthread
CC = clang $(CFLAGS)

all		:httpserver

httpserver	:httpserver.o server.o drone.o
	$(CC) -o httpserver httpserver.o server.o drone.o

httpserver.o	:httpserver.c
	$(CC) -c httpserver.c

server.o	:server.c server.h
	$(CC) -c server.c server.h

drone.o		:drone.c drone.h
	$(CC) -c drone.c drone.h

clean:
	rm -rf httpserver httpserver.o server.o drone.o server.h.gch drone.h.gch
