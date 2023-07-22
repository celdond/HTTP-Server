#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "drone.h"
#include "util.h"

#define OPTIONS "t:"
#define REQUESTS "./requests"
#define FILES "./request_files"
#define DEFAULT 2

pthread_mutex_t pc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_sig = PTHREAD_COND_INITIALIZER;
pthread_cond_t full_sig = PTHREAD_COND_INITIALIZER;
struct link_list *reference = NULL;

int acquire_file(struct threa *t, char *file_name, char verb) {
  int index = -1;
  int type = 0;
  pthread_mutex_lock(&(t->file_lock));
  for (int i = 0; i < t->thread_count; i++) {
    if (!(t->files[i][0]) && index == -1) {
      index = i;
    }
    if (t->files[i][0] == file_name[0]) {
      if (strncmp(t->files[i], file_name, 254) == 0) {
        type = 1;
        index = i;
        break;
      }
    }
  }
  if (type != 1) {
    strncpy(t->files[index], file_name, 255);
  }
  t->wanters[index] += 1;
  pthread_mutex_unlock(&(t->file_lock));
  int iter = index;
  if (verb == 'G') {
    pthread_rwlock_rdlock(&t->l[index]);
  } else {
    pthread_rwlock_wrlock(&t->l[index]);
  }
  return iter;
}

void drop_file(struct threa *t, int iter) {
  pthread_mutex_lock(&(t->file_lock));
  t->wanters[iter] -= 1;
  if (t->wanters[iter] == 0) {
    for (int i = 0; i < 255; i++) {
      t->files[iter][i] = '\0';
    }
  }
  pthread_mutex_unlock(&(t->file_lock));
  pthread_rwlock_unlock(&t->l[iter]);
  return;
}

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
      thread_op->work_buffer[thread_op->in].method = 'R';
      thread_op->in = (thread_op->in + 1) % thread_op->max_count;
      thread_op->work_buffer[thread_op->in].file = NULL;
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

    if (reference != NULL) {
    	delete_list(reference);
    }
    exit(EXIT_SUCCESS);
  }
}

ssize_t file_size(int filefd) {
  struct stat *stat_log = (struct stat *)calloc(1, sizeof(struct stat));
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

int file_check(char *file) {
  if (access(file, F_OK) != 0) {
    if (errno == EACCES) {
      perror("Forbidden\n");
    } else if (errno == ENOENT) {
      perror("File Not Found\n");
    } else {
      perror("Failure\n");
    }
    return -1;
  }
  return 1;
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

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    err(EXIT_FAILURE, "Socket could not bind.");
  }

  return sock;
}

int send_request(int conn, char *method, char *file_name) {
  char *pack = (char *)calloc(100, sizeof(char));
  if (!(pack)) {
    return -1;
  }

  snprintf(pack, 100, "%s /%s HTTP/1.0\r\n\r\n", method, file_name);

  if (send(conn, pack, 100, 0) == -1) {
    perror("Send Error\n");
    free(pack);
    return -1;
  }
  free(pack);
  return 1;
}

ssize_t reader(int connection_port, char *buffer, ssize_t size) {
        ssize_t i = 0;
        char terminate = '\0';
        ssize_t read = 0;

        while ((i < size - 1) && (terminate != '\n')) {
                read = recv(connection_port, &terminate, 1, 0);
                if (read > 0) {
                        buffer[i] = terminate;
                        i++;
                } else {
                        terminate = '\n';
                }
        }
        if (buffer[0] == '\r') {
                return 0;
        }
        buffer[i] = '\0';
        return i;
}

ssize_t grab_length(char *buffer, ssize_t i) {
    ssize_t l = 0;
    for (;; i++) {
        if (buffer[i] == ' ') {
            return -1;
        }
        if (isdigit((unsigned char) buffer[i]) == 0) {
            break;
        } else {
            l *= 10;
            l += buffer[i] - '0';
        }
    }
    return l;
}

int header_check(char *buffer, ssize_t size) {
    ssize_t x = 0;
    while (x < size) {
        if (buffer[x] == ':') {
            break;
        }
        if (buffer[x] == ' ') {
            return -1;
        }
        x++;
    }

    char c = buffer[x];
    while (c != '\r') {
        if (c == ' ') {
            return -1;
        }
        x++;
        c = buffer[x];
    }
    return x;
}

int check_response(int connfd) {
	char *buffer = (char *)calloc(1024, sizeof(char));

	ssize_t size = reader(connfd, buffer, 1024);
	int i = 0;
	int j = 0;
	while(!isspace((int)(buffer[i])) && (i < size)) {
                i++;
        }

	char *code = (char *)calloc(4, sizeof(char));
	j = 0;
	i++;
	while(!isspace((int)(buffer[i])) && (i < size)) {
		if (j > 2) {
			free(code);
			free(buffer);
			return -1;
		}
		code[j] = buffer[i];
		i++;
		j++;
	}
	code[j] = '\0';
	if (strncmp(code, "200", 3) != 0) {
                free(code);
                free(buffer);
                return -1;
        }

	// Grab First Header
	int length = 0;
       while ((size = reader(connfd, buffer, 1024)) > 0) {
                if (size > 16) {
                        if (strncmp(buffer, "Content-Length: ", 16) == 0) {
                                length = grab_length(buffer, 16);
                                if (length < 0) {
					free(code);
                                        free(buffer);
                                        return -1;
                                }
                        }
                } else {
                        if (header_check(buffer, size) < 0) {
                                free(buffer);
                                free(code);
                                return -1;
                        }
                }
        }
	free(code);
	free(buffer);
	return length;
}

void print_response(int connfd, char *file_name, struct threa *t) {
	int length = 0;
	if ((length = check_response(connfd)) < 0) {
		return;
	}
  char *path = (char *)calloc(255, sizeof(char));
  strncpy(path, "./results/", 10);

  int i = 0;
  int j = 10;
  while (!isspace((int)(file_name[i])) && ((j < 254) && (i < 254))) {
    path[j] = file_name[i];
    i++;
    j++;
  }
  path[j] = '\0';
  int lock_index = acquire_file(t, path, 'P');
  if (access(file_name, F_OK) != 0) {
    if (errno == EACCES) {
      drop_file(t, lock_index);
      perror("Forbidden\n");
      return;
    }
  }

  int filefd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (filefd == -1) {
    drop_file(t, lock_index);
    return;
  }

  char *buffer = (char *)calloc(4096, sizeof(char));
  ssize_t out, in = 0, to_go = length;
  while (to_go > 0) {
  	if ((in = recv(connfd, buffer, 4096, 0)) < 0) {
  		close(filefd);
		drop_file(t, lock_index);
		free(buffer);
		return;
  	}
	out = write(filefd, buffer, in);
	to_go -= out;
  }

  close(filefd);
  drop_file(t, lock_index);
  free(buffer);
  return;
}

int head_client(int conn, char *file_name) {
  int r = send_request(conn, "HEAD", file_name);
  return r;
}

int get_client(int conn, char *file_name) {
  int r = send_request(conn, "GET", file_name);
  return r;
}

int put_client(int conn, char *file_name) {
  char *path = (char *)calloc(1024, sizeof(char));
  if (!(path)) {
    return -1;
  }
  strncpy(path, "./request_files/", 16);
  int j = 16;
  int i = 0;
  while (!isspace((int)(file_name[i])) && ((j < 254))) {
    path[j] = file_name[i];
    i++;
    j++;
  }

  if (file_check(path) < 0) {
    free(path);
    return -1;
  }
  int filefd = open(path, O_RDONLY);
  if (filefd == -1) {
    perror("File cannot be opened\n");
    free(path);
    return -1;
  }
  ssize_t size = file_size(filefd);
  if (size == -1) {
    perror("Error opening file\n");
    close(filefd);
    return -1;
  } else if (size < -1) {
    perror("Forbidden\n");
    close(filefd);
    return -1;
  }

  char *pack = (char *)calloc(1024, sizeof(char));
  if (!(pack)) {
    free(path);
    close(filefd);
    return -1;
  }

  int s =
      snprintf(pack, 1024, "PUT /%s HTTP/1.0\r\nContent-Length: %zu\r\n\r\n",
               file_name, size);

  if (send(conn, pack, s, 0) == -1) {
    perror("Send Error\n");
    close(filefd);
    free(pack);
    return -1;
  }
  free(pack);

  char *buffer = (char *)calloc(4096, sizeof(char));
  ssize_t read_bytes;
  while (size > 0) {
    read_bytes = read(filefd, buffer, 4096);
    size -= read_bytes;
    if (send(conn, buffer, read_bytes, 0) < 0) {
      perror("Send Error\n");
      close(filefd);
      free(buffer);
      return -1;
    }
  }

  free(buffer);
  close(filefd);
  return 1;
}

int delete_client(int conn, char *file_name) {
  int r = send_request(conn, "DELETE", file_name);
  return r;
}

void *consumers(void *thread_storage) {
  struct threa *t = thread_storage;
  int connfd = t->connfd;
  int connection;
  char method;
  char *file_name;
  while (1) {
    if (pthread_mutex_lock(&pc_lock) != 0) {
      perror("Mutex Error");
      continue;
    }

    while (t->count == 0) {
      pthread_cond_wait(&empty_sig, &pc_lock);
    }
    method = t->work_buffer[t->out].method;
    if (method != 'R') {
    	file_name = *t->work_buffer[t->out].file;
    } else {
	file_name = NULL;
    }
    t->out = (t->out + 1) % t->max_count;
    t->count -= 1;
    pthread_cond_signal(&full_sig);

    pthread_mutex_unlock(&pc_lock);

    connection = create_client_socket(connfd);
    if (method == 'H') {
      head_client(connection, file_name);
    } else if (method == 'G') {
      get_client(connection, file_name);
      print_response(connection, file_name, t);
    } else if (method == 'D') {
      delete_client(connection, file_name);
    } else if (method == 'P') {
      put_client(connection, file_name);
    } else {
      return NULL;
    }
    close(connection);
  }
}

int serve_requests(struct threa *t) {
  struct dirent *d;
  DIR *directory;
  FILE *f;
  char *filename = (char *)calloc(1024, sizeof(char));
  struct link_list *l = create_list();
  reference = l;

  if ((directory = opendir(REQUESTS)) == NULL) {
    free(filename);
    delete_list(l);
    err(EXIT_FAILURE, "Cannot access required folder");
  }

  size_t buffer_size = 255;
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
      char *method = (char *)calloc(7, sizeof(char));
      while (buffer[buffer_iterator] != ',') {
        method[method_iterator] = buffer[buffer_iterator];
        method_iterator++;
        buffer_iterator++;
      }
      method[method_iterator] = '\0';
      struct node *n = insert_node(l);
      if (strncmp(method, "HEAD", 4) == 0) {
        n->command = 'H';
      } else if (strncmp(method, "GET", 3) == 0) {
        n->command = 'G';
      } else if (strncmp(method, "DELETE", 6) == 0) {
        n->command = 'D';
      } else if (strncmp(method, "PUT", 3) == 0) {
        n->command = 'P';
      } else {
        free(method);
        continue;
      }

      char *path = (char *)calloc(255, sizeof(char));
      if (n->command != '0') {
        buffer_iterator++;
        ssize_t path_iterator = 0;
        while (((buffer[buffer_iterator] != '\n') &&
                (buffer[buffer_iterator] != '\0')) &&
               (buffer_iterator < 255)) {
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
  while (file_iterator != NULL) {
    if (pthread_mutex_lock(&pc_lock) != 0) {
      perror("Mutex Error");
      continue;
    }
    while (t->count == t->max_count) {
      pthread_cond_wait(&full_sig, &pc_lock);
    }
    t->work_buffer[t->in].method = file_iterator->command;
    t->work_buffer[t->in].file = &file_iterator->file_name;
    t->in = (t->in + 1) % t->max_count;
    t->count += 1;
    pthread_cond_signal(&empty_sig);

    pthread_mutex_unlock(&pc_lock);
    file_iterator = file_iterator->next;
  }

  sigterm_handler(SIGTERM);
  fprintf(stderr, "Reached Baby\n");
  delete_list(l);
  return 0;
}

int main(int argc, char *argv[]) {
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

  struct threa *thread_storage =
      create_thread_sheet(threads, 1024, connection_port);
  if (thread_storage == NULL) {
    return EXIT_FAILURE;
  }
  thread_op = thread_storage;

  signal(SIGPIPE, SIG_IGN);
  signal(SIGTERM, sigterm_handler);
  signal(SIGINT, sigterm_handler);

  pthread_t thread_temp[threads];
  thread_list = thread_temp;
  int rc = 0;
  for (int iter = 0; iter < threads; iter++) {
    rc = pthread_create(&(thread_temp[iter]), NULL, consumers,
                        thread_storage) != 0;
    if (rc != 0) {
      return EXIT_FAILURE;
    }
  }
  serve_requests(thread_storage);
  return 0;
}
