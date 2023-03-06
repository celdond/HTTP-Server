#include "server.h"

ssize_t file_size(int filefd) {
    struct stat *stat_log = (struct stat *) calloc(1, sizeof(struct stat));
    if (!(stat_log)) {
        return -1;
    }
    if (fstat(filefd, stat_log) != 0) {
        free(stat_log);
        return -1;
    }
    if (!(S_ISREG(stat_log->st_mode))) {
        free(stat_log);
        return -2;
    }
    ssize_t size = stat_log->st_size;
    free(stat_log);
    return size;
}

int file_check(char *file, int connfd) {
    if (access(file, F_OK) != 0) {
        if (errno == EACCES) {
            send_message(connfd, 403, "Forbidden", 0);
        } else if (errno == ENOENT) {
            send_message(connfd, 404, "File Not Found", 0);
        } else {
            send_message(connfd, 500, "Internal Server Error", 0);
        }
        return -1;
    }
    return 1;
}

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
	if (file_check(file_name, connfd) < 0) {
    	    return;
    	}
    	int filefd = open(file_name, O_RDONLY);
    	if (filefd == -1) {
    	    send_message(connfd, 403, "Forbidden", 0);
    	    return;
    	}
    	ssize_t size = file_size(filefd);
    	if (size == -1) {
        	send_message(connfd, 500, "Internal Server Error", 0);
        	close(filefd);
        	return;
    	} else if (size < -1) {
        	send_message(connfd, 403, "Forbidden", 0);
        	close(filefd);
        	return;
    	}

	send_message(connfd, 200, "OK", size);
	close(filefd);
	return;
}

void get_file(int connfd, char *file_name, char *buffer) {
    if (file_check(file_name, connfd) < 0) {
            return;
    }
    int filefd = open(file_name, O_RDONLY);
    if (filefd == -1) {
        send_message(connfd, 403, "Forbidden", 0);
        return;
    }
    ssize_t size = file_size(filefd);
    if (size == -1) {
        send_message(connfd, 500, "Internal Server Error", 0);
        close(filefd);
        return;
    } else if (size < -1) {
        send_message(connfd, 403, "Forbidden", 0);
        close(filefd);
        return;
    }

    ssize_t i = size;

    send_message(connfd, 200, "OK", size);
    ssize_t read_b;
    while (i > 0) {
        read_b = read(filefd, buffer, 4096);
        i -= read_b;
        if (send(connfd, buffer, read_b, 0) < 0) {
            close(filefd);
            return;
        }
    }

    close(filefd);
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
