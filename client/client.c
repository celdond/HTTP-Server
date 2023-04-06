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

void send_request(int conn, char *method, char *file_name) {
  char *pack = (char *)calloc(100, sizeof(char));
  if (!(pack)) {
    return;
  }

  snprintf(pack, 100, "%s /%s HTTP/1.0\r\n\r\n", method, file_name);

  if (send(conn, pack, 100, 0) == -1) {
    perror("Send Error\n");
  }
  free(pack);
  return;
}

void head_client(int conn, char *file_name) {
  send_request(conn, "HEAD", file_name);
  return;
}

void get_client(int conn, char *file_name) {
  send_request(conn, "GET", file_name);
  return;
}

void put_client(int conn, char *file_name) {
  char *path = (char *)calloc(1024, sizeof(char));
  if (!(path)) {
    return;
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
    return;
  }
  int filefd = open(path, O_RDONLY);
  if (filefd == -1) {
    free(path);
    return;
  }
  ssize_t size = file_size(filefd);

  char *pack = (char *)calloc(1024, sizeof(char));
  if (!(pack)) {
    free(path);
    close(filefd);
    return;
  }

  snprintf(pack, 1024, "PUT /%s HTTP/1.0\r\nContent-Length: %zu\r\n", file_name,
           size);

  if (send(conn, pack, 1024, 0) == -1) {
    perror("Send Error\n");
  }
  free(pack);

  ssize_t out = 0, in = 0, to_go = size;
  char *buffer = (char *)calloc(4096, sizeof(char));
  while (to_go > 0) {
    if ((in = recv(conn, buffer, 4096, 0)) < 0) {
      perror("Bad Request\n");
      close(filefd);
      free(buffer);
      return;
    }
    out = write(filefd, buffer, in);
    to_go -= out;
  }

  free(buffer);
  close(filefd);
  return;
}

void delete_client(int conn, char *file_name) {
  send_request(conn, "DELETE", file_name);
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
  while (file_iterator != NULL) {
    int connection = create_client_socket(conn);
    if (file_iterator->command == 'H') {
      head_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'G') {
      get_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'D') {
      delete_client(connection, file_iterator->file_name);
    } else if (file_iterator->command == 'P') {
      put_client(connection, file_iterator->file_name);
    } else {
      file_iterator = file_iterator->next;
    }
    while ((in = recv(connection, response, 1024, 0)) > 0) {
      printf("%s", response);
    }
    close(connection);
    file_iterator = file_iterator->next;
  }

  free(response);
  delete_list(l);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    err(EXIT_FAILURE, "Too many input parameters.");
  }
  char *s;
  int port = strtol(argv[1], &s, 10);
  if (port <= 0 || *s != '\0') {
    err(EXIT_FAILURE, "Invalid Port");
  }
  serve_requests(port);
  return 0;
}
