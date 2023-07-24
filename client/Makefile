CFLAGS = -Wall -Wextra -pedantic -Wshadow -pthread -O2 -g
CC = clang $(CFLAGS)

all		:client

client		:client.o util.o drone.o communication.o
	$(CC) -o client client.o util.o drone.o communication.o

client.o	:client.c
	$(CC) -c client.c

communication.o	:communication.c
	$(CC) -c communication.c

util.o		:util.c util.h
	$(CC) -c util.c util.h

drone.o		:drone.c drone.h
	$(CC) -c drone.c drone.h

clean:
	rm -rf client client.o util.o util.h.gch drone.o drone.h.gch communication.o communication.h.gch
