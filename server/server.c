#include "server.h"

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
    if (!t->files[i][0] && index == -1) {
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

// head:
// process a head request
// connfd: connection decriptor
// file_name: name of the desired file
// t: thread sheet
void head(int connfd, char *file_name, struct threa *t) {
  int lock_index = acquire_file(t, file_name, 'G');
  if (file_check(file_name, connfd) < 0) {
    drop_file(t, lock_index);
    return;
  }
  int filefd = open(file_name, O_RDONLY);
  if (filefd == -1) {
    drop_file(t, lock_index);
    send_message(connfd, 403, "Forbidden", 0);
    return;
  }
  ssize_t size = file_size(filefd);
  if (size == -1) {
    close(filefd);
    drop_file(t, lock_index);
    send_message(connfd, 500, "Internal Server Error", 0);
    return;
  } else if (size < -1) {
    close(filefd);
    drop_file(t, lock_index);
    send_message(connfd, 403, "Forbidden", 0);
    return;
  }

  close(filefd);
  drop_file(t, lock_index);
  send_message(connfd, 200, "OK", size);
  return;
}

// get_file:
// process a get request
// connfd: connection descriptor
// file_name: name of the desired file
// t: thread sheet
void get_file(int connfd, char *file_name, struct threa *t) {
  int lock_index = acquire_file(t, file_name, 'G');
  if (file_check(file_name, connfd) < 0) {
    return;
  }
  char *buffer = (char *)calloc(4096, sizeof(char));
  int filefd = open(file_name, O_RDONLY);
  if (filefd == -1) {
    drop_file(t, lock_index);
    send_message(connfd, 403, "Forbidden", 0);
    free(buffer);
    return;
  }
  ssize_t size = file_size(filefd);
  if (size == -1) {
    close(filefd);
    drop_file(t, lock_index);
    send_message(connfd, 500, "Internal Server Error", 0);
    free(buffer);
    return;
  } else if (size < -1) {
    close(filefd);
    drop_file(t, lock_index);
    send_message(connfd, 403, "Forbidden", 0);
    free(buffer);
    return;
  }

  ssize_t i = size;

  send_message(connfd, 200, "OK", size);
  ssize_t read_b = 0;
  while (i > 0) {
    read_b = read(filefd, buffer, 4096);
    i -= read_b;
    if (send(connfd, buffer, read_b, 0) < 0) {
      close(filefd);
      drop_file(t, lock_index);
      free(buffer);
      return;
    }
  }

  close(filefd);
  drop_file(t, lock_index);
  free(buffer);
  return;
}

// put_file:
// processes a put request
// connfd: connection descriptor
// file_name: name of the desired file
// size: size of the incoming file in bytes
// t: thread sheet
void put_file(int connfd, char *file_name, ssize_t size, struct threa *t) {
  int result_message = 200;
  int lock_index = acquire_file(t, file_name, 'P');
  if (access(file_name, F_OK) != 0) {
    if (errno == EACCES) {
      drop_file(t, lock_index);
      send_message(connfd, 403, "Forbidden", 0);
      return;
    } else if (errno == ENOENT) {
      result_message = 201;
    }
  }

  int filefd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (filefd == -1) {
    drop_file(t, lock_index);
    send_message(connfd, 500, "Internal Server Error", 0);
    return;
  }
  ssize_t out = 0, in = 0, to_go = size;
  char *buffer = (char *)calloc(4096, sizeof(char));
  while (to_go > 0) {
    if ((in = recv(connfd, buffer, 4096, 0)) < 0) {
      close(filefd);
      drop_file(t, lock_index);
      send_message(connfd, 400, "Bad Request", 0);
      free(buffer);
      return;
    }
    out = write(filefd, buffer, in);
    to_go -= out;
  }
  close(filefd);
  drop_file(t, lock_index);
  if (result_message == 201) {
    send_message(connfd, 201, "Created", 0);
  } else {
    send_message(connfd, 200, "OK", 0);
  }
  free(buffer);
  return;
}

// delete_file:
// processes a delete request
// connfd: connection descriptor
// file_name: name of the desired file
// t: thread sheet
void delete_file(int connfd, char *file_name, struct threa *t) {
  int lock_index = acquire_file(t, file_name, 'P');
  if (file_check(file_name, connfd) < 0) {
    drop_file(t, lock_index);
    return;
  }

  int r = remove(file_name);
  drop_file(t, lock_index);
  if (r == 0) {
    send_message(connfd, 200, "OK", 0);
  } else {
    send_message(connfd, 500, "Internal Server Error", 0);
  }
  return;
}

// send_message:
// sends a response message in HTTP form
// connfd: connection descriptor to send message to
// mess_num: HTTP protocol code
// message: message to go with the code
// size: the length of the content to be sent in bytes
void send_message(int connfd, int mess_num, char *message, ssize_t size) {
  char *pack = (char *)calloc(100, sizeof(char));
  if (!(pack)) {
    return;
  }
  int s = 0;
  if (size == 0) {
    s = snprintf(pack, 100, "HTTP/1.0 %d %s\r\nContent-Length: %lu\r\n\r\n%s\n",
                 mess_num, message, strlen(message) + 1, message);
  } else {
    s = snprintf(pack, 100, "HTTP/1.0 %d %s\r\nContent-Length: %lu\r\n\r\n",
                 mess_num, message, size);
  }
  if (s <= 0) {
    free(pack);
    return;
  }

  send(connfd, pack, s, 0);
  free(pack);
  return;
}
