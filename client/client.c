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
#include <sys/types.h>
#include <unistd.h>

#include "communication.h"
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

// sigterm handler:
// Handles Clean-Up on Program Termination and the End of the Program
// Sig: Input Termination Signal
static void sigterm_handler(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {

    // Sends All Threads a method to terminate their routine
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

    // Ensure Threads are Terminated Properly with join
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

// create client socket:
// Creates a connection socket to the specified server
// connection port: Server Port Address
// returns: socket descriptor
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

// consumers
// Thread Functionality for processing requests
// thread storage: threa struct
void *consumers(void *thread_storage) {
  struct threa *t = thread_storage;
  int connfd = t->connfd;
  int connection;
  char method;
  char *file_name;

  while (1) {

    // Lock to Receive Dished Request
    // Waits for Requests to be served
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
    // End of Locked Block

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

// serve requests:
// producer function to serve requests in the requests folder
// t: threa struct for storing thread references
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

  // Parse Requests in Requests directory
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

    // Iterate through requests in a file
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

  // Serve Requests to Workers
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
    return EXIT_FAILURE;
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

  // Thread Creation
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
