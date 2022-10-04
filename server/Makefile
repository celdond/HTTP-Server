CFLAGS = -Wall -Wextra -pedantic -Wshadow
CC = clang $(CFLAGS)

all		:httpserver

httpserver	:httpserver.o
	$(CC) -o httpserver httpserver.o

httpserver.o	:httpserver.c
	$(CC) -c httpserver.c

clean:
	rm -rf httpserver httpserver.o
