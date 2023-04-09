#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

#define REQUESTS "./requests"
#define FILES "./request_files"
#define DEFAULT 1

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
  strncpy(path, "request_files/", 14);
  int j = 14;
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
  char *response = (char *)calloc(1024, sizeof(char));
  ssize_t in = 0;
  int r = -1;
  while (file_iterator != NULL) {
    int connection = create_client_socket(conn);
    if (file_iterator->command == 'H') {
      r = head_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'G') {
      r = get_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'D') {
      r = delete_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'P') {
      r = put_client(connection, file_iterator->file_name);
    } else {
      file_iterator = file_iterator->next;
    }
    if (r > 0) {
      while ((in = recv(connection, response, 1024, 0)) > 0) {
        printf("%s", response);
      }
    }
    close(connection);
    file_iterator = file_iterator->next;
  }

  free(response);
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

  struct threa *thread_storage = create_thread_sheet(threads, 1024);
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
  serve_requests(port);
  return 0;
}
