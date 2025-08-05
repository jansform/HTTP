#ifndef SETTING_H
#define SETTING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <time.h>

#define BUFFER_SIZE 4096  // 增大缓冲区，适应长请求
#define MAX_EVENTS 1024
#define MAX_REQUEST_LEN 8192  // 最大请求长度

typedef struct Task{
    void (*function)(void *);
    void *arg;
    struct Task *next;
    int ep_fd;  // 新增：传递epoll句柄，用于重新添加连接
} Task;

typedef struct {
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

#endif // SETTING_H

