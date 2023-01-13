CFLAGS = -Wall -Wextra -pedantic -Wshadow
CC = clang $(CFLAGS)

all		:httpserver

httpserver	:httpserver.o server.o
	$(CC) -o httpserver httpserver.o server.o

httpserver.o	:httpserver.c
	$(CC) -c httpserver.c

server.o	:server.c server.h
	$(CC) -c server.c server.h

clean:
	rm -rf httpserver httpserver.o server.o server.h.gch
