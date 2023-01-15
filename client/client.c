#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <err.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

#include "util.h"

#define REQUESTS "./requests"
#define FILES "./request_files"

void send_request(int conn, char *method, char *file_name) {
    char *pack = (char *) calloc(100, sizeof(char));
    if (!(pack)) {
        return;
    }
    ssize_t s = 0;
    s = snprintf(pack, 100, "%s /%s HTTP/1.0\r\n\r\n", method, file_name);

    send(conn, pack, s, 0);
    free(pack);
    return;
}

void head_client(int conn, char *file_name) {
	send_request(conn, "HEAD", file_name);
	return;
}

int serve_requests(int conn) {
	struct dirent *d;
	DIR *directory;
	FILE *f;
	char *filename = (char *)calloc(1024, sizeof(char));
	struct link_list *l = create_list();

	if ((directory = opendir(REQUESTS)) == NULL) {
		free(filename);
		delete_list(l);
		err(EXIT_FAILURE, "Cannot access required folder");
	}

	size_t buffer_size = 68;
	ssize_t count = 0;
	char *buffer = (char *)calloc(buffer_size, sizeof(char));
	while ((d = readdir(directory)) != NULL) {
		memset(filename, 0, 1024);
		strcat(filename, "./requests/");
		strcat(filename, d->d_name);
		f = fopen(filename, "r+");
		if (!f) {
			if (errno == EISDIR) {
				continue;
			}
			break;
		}
		while (1) {
			memset(buffer, 0, buffer_size);
			count = getline(&buffer, &buffer_size, f);
			if (count < 3) {
				break;
			}
			ssize_t method_iterator = 0, buffer_iterator = 0;
			char *method = (char *)calloc(5, sizeof(char));
			while (buffer[buffer_iterator] != ',') {
				method[method_iterator] = buffer[buffer_iterator];
				method_iterator++;
				buffer_iterator++;
			}
			method[method_iterator] = '\0';
			struct node *n = insert_node(l);
			if (strcmp(method, "HEAD")) {
				n->command = 'H';
			} else {
				free(method);
				continue;
			}

			char *path = (char *)calloc(255, sizeof(char));
			if (n->command != '0') {
				buffer_iterator++;
				ssize_t path_iterator = 0;
				while (buffer[buffer_iterator] != '0') {
					path[path_iterator] = buffer[buffer_iterator];
					path_iterator++;
					buffer_iterator++;
				}
				n->file_name = path;
			}
			free(method);
		}
		fclose(f);
	}
	free(filename);
	free(buffer);
	closedir(directory);

	struct node *file_iterator = l->head;
	char *response = (char *)calloc(1024, sizeof(char));
	ssize_t in = 0;
	while(file_iterator) {
		if (file_iterator->command == 'H') {
			head_client(conn, file_iterator->file_name);
		}
		while((in = recv(conn, response, 1024, 0)) > 0) {
                	printf("%s", response);
        	}
		file_iterator = file_iterator->next;
	}

	free(response);
	delete_list(l);
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
