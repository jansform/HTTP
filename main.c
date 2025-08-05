#include "threadpool.h"
#include "run.h"

int main(int argc, char const *argv[]) {
    ThreadPool *pool = threadpool_create(6, 20);
    if (!pool) {
        perror("threadpool_create failed");
        return EXIT_FAILURE;
    }

    int ep_fd = epoll_create1(0);
    if (ep_fd < 0) {
        perror("epoll_create1 failed");
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }

    int s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd < 0) {
        perror("socket failed");
        close(ep_fd);
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(s_fd);
        close(ep_fd);
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }

    struct epoll_event event;
    event.data.fd = s_fd;
    event.events = EPOLLIN;
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, s_fd, &event) < 0) {
        perror("epoll_ctl add s_fd failed");
        close(s_fd);
        close(ep_fd);
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(s_fd);
        close(ep_fd);
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }

    if (listen(s_fd, 1024) < 0) {
        perror("listen failed");
        close(s_fd);
        close(ep_fd);
        threadpool_destroy(pool);
        return EXIT_FAILURE;
    }
    printf("服务器启动，监听端口 9000...\n");

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int ready = epoll_wait(ep_fd, events, MAX_EVENTS, -1);
        if (ready < 0) {
            if (errno == EINTR) continue;  // 信号中断重试
            perror("epoll_wait failed");
            break;
        }

        for (size_t i = 0; i < (size_t)ready; i++) {
            int fd = events[i].data.fd;
            if (fd == s_fd) {  // 新连接
                struct sockaddr_in c_addr;
                socklen_t len = sizeof(c_addr);
                int new_fd = accept(s_fd, (struct sockaddr*)&c_addr, &len);
                if (new_fd < 0) {
                    perror("accept failed");
                    continue;
                }

                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &c_addr.sin_addr, ip, sizeof(ip));
                printf("新连接 %d, %s:%d\n", new_fd, ip, ntohs(c_addr.sin_port));

                struct epoll_event ev;
                ev.data.fd = new_fd;
                ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
                if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, new_fd, &ev) < 0) {
                    perror("epoll_ctl add new_fd failed");
                    close(new_fd);
                }
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  // 连接异常
                printf("连接 %d 异常断开\n", fd);
                epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            } else if (events[i].events & EPOLLIN) {  // 可读事件
                // 从epoll移除fd（处理期间不再监听）
                epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, NULL);
                
                int *client_fd = malloc(sizeof(int));
                *client_fd = fd;
                // 传递epoll句柄，用于keep-alive重新添加
                threadpool_add_task(pool, run, client_fd, ep_fd);
            }
        }
    }

    close(s_fd);
    close(ep_fd);
    threadpool_destroy(pool);
    return 0;
}