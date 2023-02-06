#ifndef __DRONE_H__
#define __DRONE_H__

struct threa *thread_op;

struct threa {
	int *work_buffer;
	int in;
	int out;
	int count;
	int thread_count;
	int max_count;
};

struct threa *create_thread_sheet(int thread_count, int max_count);

void free_threa(struct threa *t);
#endif
