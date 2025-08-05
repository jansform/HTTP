#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "setting.h"

void *threadpool_worker(void *arg);
void threadpool_destroy(ThreadPool *pool);
ThreadPool *threadpool_create(int thread_count, int max_queue_size);
void threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg, int ep_fd);

#endif