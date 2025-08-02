#include "threadpool.h"

ThreadPool *threadpool_create(int thread_count, int max_queue_size) {
    // 判断传入的线程数和队列最大长度是否小于等于0，如果是，则返回NULL
    if (thread_count <= 0 || max_queue_size <= 0) {
        return NULL;
    }

    // 分配ThreadPool结构体的内存空间
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        return NULL;
    }

    // 分配线程数目的内存空间
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    // 初始化互斥锁
    pthread_mutex_init(&(pool->lock), NULL);
    // 初始化条件变量
    pthread_cond_init(&(pool->notify), NULL);
    // 初始化队列头尾指针
    pool->queue_head = pool->queue_tail = NULL;
    // 设置线程数目
    pool->thread_count = thread_count;
    // 设置队列长度
    pool->queue_size = 0;
    // 设置队列最大长度
    pool->max_queue_size = max_queue_size;
    // 设置关闭标志
    pool->shutdown = 0;

    // 创建线程
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            // 如果创建线程失败，则销毁线程池并返回NULL
            threadpool_destroy(pool);
            return NULL;
        }
    }
    // 返回线程池
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
    // 定义一个指向ThreadPool的指针
    ThreadPool *pool = (ThreadPool *)arg;
    // 定义一个指向Task的指针
    Task *task;

    // 无限循环
    while (1) {
        // 加锁
        pthread_mutex_lock(&(pool->lock));

        // 如果队列大小为0且线程池未关闭，则等待
        while (pool->queue_size == 0 && !pool->shutdown) {
            // 等待条件变量
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // 如果线程池已关闭，则解锁并退出线程
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 从队列中取出任务
        task = pool->queue_head;
        pool->queue_head = task->next;
        pool->queue_size--;

        // 解锁
        pthread_mutex_unlock(&(pool->lock));

        // 执行任务
        (*(task->function))(task->arg);
        // 释放任务
        free(task);
    }

    // 返回NULL
    return NULL;
}

// 销毁线程池
void threadpool_destroy(ThreadPool *pool) {
    // 如果线程池为空，直接返回
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

    // 释放任务队列
    Task *task;
    while (pool->queue_head != NULL) {
        task = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(task);
    }

    // 销毁互斥锁
    pthread_mutex_destroy(&(pool->lock));
    // 销毁条件变量
    pthread_cond_destroy(&(pool->notify));
    // 释放线程池
    free(pool);
}