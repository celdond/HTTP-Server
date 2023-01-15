#include "server.h"

ssize_t reader(int connection_port, char *buffer, ssize_t size) {
	ssize_t i = 0;
	char terminate = '\0';
	ssize_t read = 0;

	while ((i < size - 1) && (terminate != '\n')) {
		read = recv(connection_port, &terminate, 1, 0);
		if (read > 0) {
			if (terminate == '\r') {
				read = recv(connection_port, &terminate, 1, MSG_PEEK);
				if ((read > 0) && (terminate == '\n')) {
					recv(connection_port, &terminate, 1, 0);
				} else {
					terminate = '\n';
				}
			}
			buffer[i] = terminate;
			i++;
		} else {
			terminate = '\n';
		}
	}
	buffer[i] = '\0';
	return i;
}

void head(int connfd, char *file_name) {
	return;
}

void send_message(int connfd, int mess_num, char *message, ssize_t size) {
    char *pack = (char *) calloc(100, sizeof(char));
    if (!(pack)) {
        return;
    }
    int s = 0;
    if (size == 0) {
        s = snprintf(pack, 100, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n%s\n", mess_num,
            message, strlen(message) + 1, message);
    } else {
        s = snprintf(
            pack, 100, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n", mess_num, message, size);
    }
    if (s <= 0) {
        free(pack);
        return;
    }

    send(connfd, pack, s, 0);
    free(pack);
    return;
}
