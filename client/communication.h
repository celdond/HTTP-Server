#include <errno.h>
#include <pthread.h>

int acquire_file(struct threa *t, char *file_name, char verb);

void drop_file(struct threa *t, int iter);

ssize_t reader(int connection_port, char *buffer, ssize_t size);

print_response(int connfd, char *file_name, struct threa *t);

int head_client(int conn, char *file_name);

int get_client(int conn, char *file_name);

int put_client(int conn, char *file_name);

int delete_client(int conn, char *file_name);
