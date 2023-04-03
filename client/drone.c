#include "drone.h"

struct threa *create_thread_sheet(int thread_count, int max_count) {
  struct threa *t = (struct threa *)calloc(1, sizeof(struct threa));
  t->work_buffer = (int *)calloc(max_count, sizeof(int));
  t->in = 0;
  t->out = 0;
  t->count = 0;
  t->thread_count = thread_count;
  t->max_count = max_count;
  return t;
}

void free_threa(struct threa *t) {
  free(t->work_buffer);
  free(t);
  return;
}
