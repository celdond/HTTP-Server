#ifndef __DRONE_H__
#define __DRONE_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct threa *thread_op;

pthread_t *thread_list;

struct request {
	char **file;
	char method;
};

struct threa {
	struct request *work_buffer;
	char **files;
	int *wanters;
	pthread_rwlock_t *l;
	pthread_mutex_t file_lock;
	int in;
	int out;
	int count;
	int thread_count;
	int max_count;
	int connfd;
};

struct threa *create_thread_sheet(int thread_count, int max_count, int connfd);

void free_threa(struct threa *t);
#endif
