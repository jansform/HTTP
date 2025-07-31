#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef struct Task {
    void (*function)(void *);  // 明确使用 void 返回类型
    void *arg;
    struct Task *next;
} Task;

typedef struct ThreadPool {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    Task *queue_head;
    Task *queue_tail;
    int thread_count;
    int queue_size;
    int max_queue_size;
    int shutdown;
} ThreadPool;

ThreadPool *threadpool_create(int thread_count, int max_queue_size);
void threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg);
void threadpool_destroy(ThreadPool *pool);
void *threadpool_worker(void *arg);

#endif