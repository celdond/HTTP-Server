#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#include "server.h"

#define OPTIONS "t:"
#define DEFAULT 1

void handle_request(int connfd) {
	char *buffer = (char *)calloc(1024, sizeof(char));
	char *method = (char *)calloc(8, sizeof(char));
	char *path = (char *)calloc(255, sizeof(char));
	ssize_t i = 0;
	ssize_t j = 0;
	ssize_t size;

	size = reader(connfd, buffer, 1024);
	while(!isspace((int)(buffer[i])) && (j < 6) && (i < size)) {
		method[j] = buffer[i];
		i++;
		j++;
	}
	method[j] = '\0';

	j = 6;
	while(isspace((int)(buffer[i])) && (i < 1024)) {
		i++;
	}

	strncpy(path, "/files", 6);

	while(!isspace((int)(buffer[i])) && (j < 254) && (i < size)) {
		path[j] = buffer[i];
		i++;
		j++;
	}
	path[j] = '\0';


	if (strncmp(method, "HEAD", 4) == 0) {
		head(connfd, path);
	} else {
		free(buffer);
		free(method);
		free(path);
		return;
	}

	free(buffer);
	free(method);
	free(path);
	return;
}

static int create_socket(int input_port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		err(EXIT_FAILURE, "Socket Failure");
	}
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(input_port);

	if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		err(EXIT_FAILURE, "Socket could not bind.");
	}

	if (listen(sock, 1) < 0) {
		err(EXIT_FAILURE, "Cannot listen to socket.");
	}

	return sock;
}

int main (int argc, char *argv[]) {
	int op = 0;
	int threads = DEFAULT;
	while ((op = getopt(argc, argv, OPTIONS)) != -1) {
		switch (op) {
		case 't':
			threads = strtol(optarg, NULL, 10);
			if (threads <= 0) {
				errx(EXIT_FAILURE, "bad number of threads");
			}
			break;
		default:
			errx(EXIT_FAILURE, "Invalid parameter");
		}
	}

	if (optind >= argc) {
		errx(EXIT_FAILURE, "Wrong number of parameters");
	}
	int connection_port;
        char *s;
        connection_port = strtol(argv[optind], &s, 10);
        if (connection_port <= 0 || *s != '\0') {
                err(EXIT_FAILURE, "Invalid Port");
        }
	
	struct threa *thread_storage = create_thread_sheet(threads, 1024);
	if (thread_storage == NULL) {
		return EXIT_FAILURE;
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	int listen = create_socket(connection_port);

	pthread_t thread
	while (1) {
		int connection = accept(listen, NULL, NULL);
		handle_request(connection);
		close(connection);
	}
	
	return EXIT_SUCCESS;
}
