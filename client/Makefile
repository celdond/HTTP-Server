CFLAGS = -Wall -Wextra -pedantic -Wshadow
CC = clang $(CFLAGS)

all		:client

client		:client.o util.o
	$(CC) -o client client.o util.o

client.o	:client.c
	$(CC) -c client.c

util.o		:util.c util.h
	$(CC) -c util.c util.h

clean:
	rm -rf client client.o util.o util.h.gch
