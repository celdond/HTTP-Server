#include "drone.h"

// create_thread_sheet_locks:
// sets a lock for every thread in the sheet
void create_thread_sheet_locks(struct threa *t, int iter) {
  pthread_rwlock_t d_lock;
  pthread_rwlock_init(&(d_lock), NULL);
  t->l[iter] = d_lock;
  return;
}

// create_thread_sheet:
// creates a struct to store thread information
// thread_count: number of threads
// max_count: size of the work buffer
// connfd: server descriptor
// return: thread sheet as a struct threa reference
struct threa *create_thread_sheet(int thread_count, int max_count, int connfd) {
  struct threa *t = (struct threa *)calloc(1, sizeof(struct threa));
  t->work_buffer = (struct request *)calloc(max_count, sizeof(struct request));
  t->files = (char **)calloc(thread_count, sizeof(char *));
  t->wanters = (int *)calloc(thread_count, sizeof(int));
  t->l = (pthread_rwlock_t *)calloc(thread_count, sizeof(pthread_rwlock_t));

  for (int i = 0; i < thread_count; i++) {
    t->files[i] = calloc(255, sizeof(char));
    pthread_rwlock_init(&t->l[i], NULL);
    create_thread_sheet_locks(t, i);
  }
  t->in = 0;
  t->out = 0;
  t->count = 0;
  t->thread_count = thread_count;
  t->connfd = connfd;
  t->max_count = max_count;
  pthread_mutex_init(&(t->file_lock), NULL);
  return t;
}

// free_threa:
// frees memory allocated to the thread sheet
// t: thread sheet to clear
void free_threa(struct threa *t) {
  free(t->work_buffer);
  free(t->wanters);
  for (int i = 0; i < t->thread_count; i++) {
    free(t->files[i]);
    pthread_rwlock_destroy(&t->l[i]);
  }
  free(t->files);
  free(t->l);
  free(t);
  return;
}
