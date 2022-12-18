#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <err.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>

#define REQUESTS "./requests"
#define FILES "./request_files"

int serve_requests(int conn) {
	struct dirent *d;
	DIR *directory;
	char *filename = (char *)calloc(1024, sizeof(char));

	if ((directory = opendir(REQUESTS)) == NULL) {
		err(EXIT_FAILURE, "Cannot access required folder");
	}
	while ((d = readdir(directory)) != NULL) {
		memset(filename, 0, 1024);
		strcat(filename, "/");
		f = fopen(d->d_name, "w");
		close(f);
	}
	closedir(directory);
	return 0;
}

static int create_client_socket(int connection_port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		err(EXIT_FAILURE, "Socket Failure");
	}
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);
	server_addr.sin_port = htons(connection_port);

	if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		err(EXIT_FAILURE, "Socket could not bind.");
	}

	return sock;
}

int main(int argc, char *argv[]) {
	int connection_port;
	if (argc != 2) {
		err(EXIT_FAILURE, "Too many input parameters.");
	}
	char *s;
	connection_port = strtol(argv[1], &s, 10);
	if (connection_port <= 0 || *s != '\0') {
		err(EXIT_FAILURE, "Invalid Port");
	}
	int connection = create_client_socket(connection_port);
	serve_requests(connection);
	return 0;
}
