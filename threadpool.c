#include "threadpool.h"

ThreadPool *threadpool_create(int thread_count, int max_queue_size) {
    if (thread_count <= 0 || max_queue_size <= 0) {
        return NULL;
    }

    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);
    pool->queue_head = pool->queue_tail = NULL;
    pool->thread_count = thread_count;
    pool->queue_size = 0;
    pool->max_queue_size = max_queue_size;
    pool->shutdown = 0;

    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

// 添加任务到线程池
void threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg) {
    // 锁定互斥锁
    pthread_mutex_lock(&(pool->lock));

    // 如果队列大小已经达到最大值，则解锁互斥锁并返回
    if (pool->queue_size >= pool->max_queue_size) {
        pthread_mutex_unlock(&(pool->lock));
        return;
    }

    // 分配任务结构体
    Task *task = (Task *)malloc(sizeof(Task));
    if (task == NULL) {
        pthread_mutex_unlock(&(pool->lock));
        return;
    }

    // 初始化任务结构体
    task->function = function;
    task->arg = arg;
    task->next = NULL;

    // 如果队列为空，则将任务添加到队列头部和尾部
    if (pool->queue_head == NULL) {
        pool->queue_head = pool->queue_tail = task;
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }
    pool->queue_size++;

    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
}

void *threadpool_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    Task *task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        while (pool->queue_size == 0 && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        task = pool->queue_head;
        pool->queue_head = task->next;
        pool->queue_size--;

        pthread_mutex_unlock(&(pool->lock));

        (*(task->function))(task->arg);
        free(task);
    }

    return NULL;
}

void threadpool_destroy(ThreadPool *pool) {
    if (pool == NULL) return;

    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    Task *task;
    while (pool->queue_head != NULL) {
        task = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(task);
    }

    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
}