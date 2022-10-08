CFLAGS = -Wall -Wextra -pedantic -Wshadow
CC = clang $(CFLAGS)

all		:client

client		:client.o
	$(CC) -o client client.o

client.o	:client.c
	$(CC) -c client.c

clean:
	rm -rf client client.o
