#include "communication.h"

// acquire_file:
// Set a lock on a file to access
// t: thread sheet
// file_name: name of the desired file
// verb: Desired access, G to Read and P to Write
// return: index of lock
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

// drop_file:
// Remove a taken lock from a file
// t: thread sheet
// iter: index of lock to drop
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

// file_size:
// check the size of the input file
// filefd: file descriptor
// return: size of the file in bytes
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

// file_check:
// check if a file is accessible
// file: name of the file to check
// return: integer where positive is accessible and negative is not
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

// send_request:
// send a request to the server
// conn: connection descriptor
// method: string to include in request
// file_name: name of the file in request
// return: integer, positive for success
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

// reader:
// reads in from the port byte by byte
// connection_port: port connection to listen to
// buffer: storage for bytes read from the port
// size: size of the buffer in bytes
// return: number of bytes read
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

// grab_length:
// grabs the length of the content length header
// buffer: storage containing the data to be read
// i: current index in buffer
// return: content-length as read by function
ssize_t grab_length(char *buffer, ssize_t i) {
  ssize_t l = 0;
  for (;; i++) {
    if (buffer[i] == ' ') {
      return -1;
    }
    if (isdigit((unsigned char)buffer[i]) == 0) {
      break;
    } else {
      l *= 10;
      l += buffer[i] - '0';
    }
  }
  return l;
}

// header_check:
// checks the headers to ensure proper format
// buffer: storage containing header to check
// size: size of the buffer in bytes
// return: integer, positive for success
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

// check_response:
// takes in the connection that is being checked
// connfd: connection descriptor
// return: integer, where positive is success
int check_response(int connfd) {
  char *buffer = (char *)calloc(1024, sizeof(char));

  ssize_t size = reader(connfd, buffer, 1024);
  int i = 0;
  int j = 0;
  while (!isspace((int)(buffer[i])) && (i < size)) {
    i++;
  }

  char *code = (char *)calloc(4, sizeof(char));
  j = 0;
  i++;
  while (!isspace((int)(buffer[i])) && (i < size)) {
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

  // Check Headers
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

// print_response:
// prints out the content sent from the connection descriptor
// connfd: connection descriptor
// file_name: name of the file to print
// t: thread sheet
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
      free(path);
      drop_file(t, lock_index);
      perror("Forbidden\n");
      return;
    }
  }

  int filefd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (filefd == -1) {
    free(path);
    drop_file(t, lock_index);
    return;
  }

  char *buffer = (char *)calloc(4096, sizeof(char));
  ssize_t out, in = 0, to_go = length;
  fprintf(stderr, "%s\n", buffer);
  while (to_go > 0) {
    if ((in = recv(connfd, buffer, 4096, 0)) < 0) {
      perror("Read Error:");
      close(filefd);
      drop_file(t, lock_index);
      free(path);
      free(buffer);
      return;
    }
    out = write(filefd, buffer, in);
    to_go -= out;
  }

  close(filefd);
  drop_file(t, lock_index);
  free(path);
  free(buffer);
  return;
}

// head_client:
// send a head request
// conn: connection descriptor
// file_name: file name to request
// return: integer, positive for success
int head_client(int conn, char *file_name) {
  int r = send_request(conn, "HEAD", file_name);
  return r;
}

// get_client:
// send a get request
// conn: connection descriptor
// file_name: file name to request
// return: integer, positive for success
int get_client(int conn, char *file_name) {
  int r = send_request(conn, "GET", file_name);
  return r;
}

// put_client:
// send a put request
// conn: connection descriptor
// file_name: file name to request
// return: integer, positive for success
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
    free(path);
    return -1;
  } else if (size < -1) {
    perror("Forbidden\n");
    close(filefd);
    free(path);
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
    free(path);
    return -1;
  }
  free(pack);

  // Send the file
  char *buffer = (char *)calloc(4096, sizeof(char));
  ssize_t read_bytes;
  while (size > 0) {
    read_bytes = read(filefd, buffer, 4096);
    size -= read_bytes;
    if (send(conn, buffer, read_bytes, 0) < 0) {
      perror("Send Error\n");
      close(filefd);
      free(buffer);
      free(path);
      return -1;
    }
  }

  free(buffer);
  free(path);
  close(filefd);
  return 1;
}

// delete_client:
// send a delete request
// conn: connection descriptor
// file_name: file name to request
// return: integer, positive for success
int delete_client(int conn, char *file_name) {
  int r = send_request(conn, "DELETE", file_name);
  return r;
}
