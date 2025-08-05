#include "threadpool.h"

// 线程工作函数
void *threadpool_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;  // 将arg转换为ThreadPool类型
    Task *task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));  // 锁定互斥锁
        // 如果队列为空且线程池未关闭，则等待通知
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

        (*(task->function))(task);  // 传递整个task
    }
    return NULL;
}

// 销毁线程池
void threadpool_destroy(ThreadPool *pool) {
    // 如果线程池为空，则直接返回
    if (pool == NULL) return;

    // 加锁
    pthread_mutex_lock(&(pool->lock));
    // 设置线程池关闭标志
    pool->shutdown = 1;
    // 唤醒所有等待的线程
    pthread_cond_broadcast(&(pool->notify));
    // 解锁
    pthread_mutex_unlock(&(pool->lock));

    // 等待所有线程结束
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // 释放线程数组
    free(pool->threads);
    // 释放任务队列中的任务
    Task *task;
    while (pool->queue_head != NULL) {
        task = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(task->arg);
        free(task);
    }

    // 销毁互斥锁
    pthread_mutex_destroy(&(pool->lock));
    // 销毁条件变量
    pthread_cond_destroy(&(pool->notify));
    // 释放线程池
    free(pool);
}

// 线程池创建
ThreadPool *threadpool_create(int thread_count, int max_queue_size) {
    // 检查传入的参数是否合法
    if (thread_count <= 0 || max_queue_size <= 0) {
        return NULL;
    }

    // 分配内存空间
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        return NULL;
    }

    // 分配内存空间
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    // 初始化互斥锁和条件变量
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
// 向线程池中添加任务
void threadpool_add_task(ThreadPool *pool, void (*function)(void *), void *arg, int ep_fd) {
    // 加锁
    pthread_mutex_lock(&(pool->lock));

    // 如果任务队列已满，则释放未添加的任务资源
    if (pool->queue_size >= pool->max_queue_size) {
        pthread_mutex_unlock(&(pool->lock));
        free(arg);  // 释放未添加的任务资源
        return;
    }

    // 创建新任务
    Task *task = (Task *)malloc(sizeof(Task));
    if (task == NULL) {
        pthread_mutex_unlock(&(pool->lock));
        free(arg);
        return;
    }

    // 初始化任务
    task->function = function;
    task->arg = arg;
    task->ep_fd = ep_fd;  // 传递epoll句柄
    task->next = NULL;

    // 将任务添加到任务队列
    if (pool->queue_head == NULL) {
        pool->queue_head = pool->queue_tail = task;
    } else {
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }
    pool->queue_size++;

    // 通知线程池中有新任务
    pthread_cond_signal(&(pool->notify));
    // 解锁
    pthread_mutex_unlock(&(pool->lock));
}