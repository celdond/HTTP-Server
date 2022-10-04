#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>

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

	int listen = create_socket(8080);

	//int connection = accept(listen, NULL, NULL);

	return EXIT_SUCCESS;
}
