#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <pthread.h>

#include "drone.h"
#include "server.h"

#define OPTIONS "t:"
#define DEFAULT 1

pthread_mutex_t pc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_sig = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_sig = PTHREAD_COND_INITIALIZER;

static void sigterm_handler(int sig) {
	if (sig == SIGTERM || sig == SIGINT) {

        	for (int i = 0; i < thread_op->thread_count; i++) {
            		if (pthread_mutex_lock(&pc_lock) != 0) {
                		perror("Mutex Error");
                		continue;
            		}
            		while (thread_op->count == thread_op->max_count) {
                		pthread_cond_wait(&full_sig, &pc_lock);
            		}
            		thread_op->work_buffer[thread_op->in] = -1;
            		thread_op->in = (thread_op->in + 1) % thread_op->max_count;
            		thread_op->count += 1;
            		pthread_cond_signal(&empty_sig);

            		pthread_mutex_unlock(&pc_lock);
        	}

        	void *data;
        	for (int i = 0; i < thread_op->thread_count; i++) {
            		pthread_join(thread_list[i], &data);
            		if (data != NULL) {
                		free(data);
            		}
        	}

        free_threa(thread_op);
        exit(EXIT_SUCCESS);
    }
}

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

	j = 5;
	while(isspace((int)(buffer[i])) && (i < 1024)) {
		i++;
	}

	strncpy(path, "files", 5);

	while(!isspace((int)(buffer[i])) && ((j < 254) && (i < size))) {
		path[j] = buffer[i];
		i++;
		j++;
	}
	path[j] = '\0';

	if (strncmp(method, "HEAD", 4) == 0) {
		head(connfd, path);
	} else if () {
		get_file(connfd, path);
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

void producer(int listenfd, struct threa *t) {
	int connfd;
	while (1) {
		connfd = accept(listenfd, NULL, NULL);
        	if (connfd < 0) {
        	    perror("accept error");
        	    continue;
        	}
        	if (pthread_mutex_lock(&pc_lock) != 0) {
        	    perror("Mutex Error");
        	    continue;
        	}
        	while (t->count == t->max_count) {
        	    pthread_cond_wait(&full_sig, &pc_lock);
        	}
        	t->work_buffer[t->in] = connfd;
        	t->in = (t->in + 1) % t->max_count;
        	t->count += 1;
        	pthread_cond_signal(&empty_sig);

        	pthread_mutex_unlock(&pc_lock);
	}
	return;
}

void *consumers(void *thread_storage) {
    int connfd;
    struct threa *t = thread_storage;
    while (1) {
        if (pthread_mutex_lock(&pc_lock) != 0) {
            perror("Mutex Error");
            continue;
        }

        while (t->count == 0) {
            pthread_cond_wait(&empty_sig, &pc_lock);
        }
        connfd = t->work_buffer[t->out];
        t->out = (t->out + 1) % t->max_count;
        t->count -= 1;
        pthread_cond_signal(&full_sig);

        pthread_mutex_unlock(&pc_lock);

        if (connfd < 0) {
            return NULL;
        }

        handle_request(connfd);
        close(connfd);
    }
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
	thread_op = thread_storage;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigterm_handler);

	int listenfd = create_socket(connection_port);

	pthread_t thread_temp[threads];
	thread_list = thread_temp;
	int rc = 0;
	for (int iter = 0; iter < threads; iter++) {
        	rc = pthread_create(&(thread_temp[iter]), NULL, consumers, thread_storage) != 0;
        	if (rc != 0) {
            		return EXIT_FAILURE;
        	}
    	}

    	producer(listenfd, thread_storage);
	
	return EXIT_SUCCESS;
}
