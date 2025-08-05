#include "setting.h"
#include "run.h"

// URL解码函数
void url_decode(char *dst, const char *src) {
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = {src[1], src[2], '\0'};
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// 路径校验
int isvalid_path(char *path){
    if(strstr(path,"../")||strstr(path,"..\\")||strstr(path,"./")||strstr(path,".\\")){
        return 0;
    }
    return 1;
}

// MIME类型
char *get_mime_type(char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) 
        return "text/html";
    if (strcmp(dot, ".txt") == 0) 
        return "text/plain";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) 
        return "image/jpeg";
    if (strcmp(dot, ".png") == 0) 
        return "image/png";
    if (strcmp(dot, ".gif") == 0) 
        return "image/gif";
    if (strcmp(dot, ".css") == 0) 
        return "text/css";
    if (strcmp(dot, ".js") == 0) 
        return "application/javascript";
    if (strcmp(dot, ".json") == 0) 
        return "application/json";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    return "application/octet-stream";
}

// 解析HTTP请求（新增keep-alive判断）
void parse_http_request(char *req, HttpRequest *parse){
    memset(parse, 0, sizeof(HttpRequest));
    parse->keep_alive = 0;  // 默认关闭
    
    // 解析第一行
    char raw_path[1024];
    sscanf(req, "%15s %1023s %15s", parse->method, raw_path, parse->version);
    url_decode(parse->path, raw_path);

    // 解析Host
    char *host_start = strstr(req, "Host:");
    if(host_start){
        host_start += 5;
        while (*host_start == ' ') host_start++;
        char *host_end = strstr(host_start, "\r\n");
        if(host_end) {
            strncpy(parse->host, host_start, host_end - host_start);
            parse->host[host_end - host_start] = '\0';
        }
    }

    // 解析Connection头，判断是否保持连接
    char *conn = strstr(req, "Connection:");
    if (conn) {
        conn += strlen("Connection:");
        while (*conn == ' ') conn++;
        char *conn_end = strstr(conn, "\r\n");
        if (conn_end) {
            char conn_val[32];
            strncpy(conn_val, conn, conn_end - conn);
            conn_val[conn_end - conn] = '\0';
            if (strcasecmp(conn_val, "keep-alive") == 0) {
                parse->keep_alive = 1;
            }
        }
    }
    // HTTP/1.1默认keep-alive
    else if (strcasecmp(parse->version, "HTTP/1.1") == 0) {
        parse->keep_alive = 1;
    }
}

// 发送HTTP响应（修复状态码）
void http_response(int sockfd, char *status, char *type, char *body) {
    char headers[BUFFER_SIZE];
    // 修复：使用传入的status作为完整状态码（如"404 Not Found"）
    snprintf(headers, sizeof(headers),
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: %s\r\n\r\n",  // 根据keep-alive动态设置
    status, type, strlen(body), 
    (strstr(status, "200") && /* 仅成功响应可能keep-alive */ 0) ? "keep-alive" : "close");
    if (send(sockfd, headers, strlen(headers), 0) < 0) {
        perror("http_response send headers failed");
        return;
    }
    if (send(sockfd, body, strlen(body), 0) < 0) {
        perror("http_response send body failed");
    }
}

// 发送文件（完善错误处理）
void send_file(int sockfd, char *path, int keep_alive) {
    if (strcmp(path, "/") == 0) {
        path = "/login.html";
    }

    if (!isvalid_path(path)) {
        http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
        return;
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s", path + 1);  // 修复路径拼接

    int fd = open(full_path, O_RDONLY);
    if(fd < 0) {
        if (errno == ENOENT) {
            http_response(sockfd, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
        } else if (errno == EACCES) {
            http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
        } else {
            http_response(sockfd, "500 Internal Server Error", "text/html", "<h1>500 Internal Server Error</h1>");
        }
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) < 0) {  // 增加fstat错误处理
        perror("fstat failed");
        close(fd);
        http_response(sockfd, "500 Internal Server Error", "text/html", "<h1>500 Internal Server Error</h1>");
        return;
    }
    
    char headers[BUFFER_SIZE];
    snprintf(headers, sizeof(headers),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: %s\r\n\r\n",  // 动态设置连接状态
    get_mime_type(full_path), st.st_size,
    keep_alive ? "keep-alive" : "close");
    
    if (send(sockfd, headers, strlen(headers), 0) < 0) {
        perror("send_file send headers failed");
        close(fd);
        return;
    }

    off_t offset = 0;
    ssize_t sent = 0;
    while (sent < st.st_size) {
        ssize_t n = sendfile(sockfd, fd, &offset, st.st_size - sent);
        if (n < 0) {
            if (errno == EINTR) continue;  // 信号中断重试
            perror("sendfile failed");
            break;
        } else if (n == 0) {
            break;  // 发送完成
        }
        sent += n;
    }
    close(fd);
}

// 线程处理函数（完善请求读取和连接管理）
void run(void *arg) {
    Task *task = (Task *)arg;
    int c_fd = *(int*)(task->arg);
    int ep_fd = task->ep_fd;  // 获取epoll句柄
    free(task->arg);  // 释放客户端fd内存
    free(task);  // 释放任务结构体

    char req[MAX_REQUEST_LEN] = {0};
    ssize_t total_len = 0;
    ssize_t len;

    // 循环读取完整请求头（直到\r\n\r\n或缓冲区满）
    while (1) {
        len = recv(c_fd, req + total_len, MAX_REQUEST_LEN - total_len - 1, 0);
        if (len < 0) {
            if (errno == EINTR) continue;  // 信号中断重试
            perror("recv failed");
            close(c_fd);
            return;
        } else if (len == 0) {  // 客户端主动关闭
            printf("client %d closed\n", c_fd);
            close(c_fd);
            return;
        }
        total_len += len;
        req[total_len] = '\0';

        // 检查是否读取到完整请求头
        if (strstr(req, "\r\n\r\n") != NULL) {
            break;
        }
        // 防止缓冲区溢出
        if (total_len >= MAX_REQUEST_LEN - 1) {
            http_response(c_fd, "413 Payload Too Large", "text/html", "<h1>413 Payload Too Large</h1>");
            close(c_fd);
            return;
        }
    }

    // 解析请求
    HttpRequest parse;
    parse_http_request(req, &parse);

    // 日志记录
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(c_fd, (struct sockaddr*)&addr, &addr_len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(addr.sin_port);

    FILE *logf = fopen("server.log", "a");
    if (logf) {
        fprintf(logf, "[%s] %s:%d 请求 %s %s\n", time_str, ip, port, parse.method, parse.path);
        fclose(logf);
    }

    // 处理/favicon.ico
    if (strcmp(parse.path, "/favicon.ico") == 0) {
        http_response(c_fd, "404 Not Found", "text/html", "");
        logf = fopen("server.log", "a");
        if (logf) {
            fprintf(logf, "[%s] %s:%d 响应 404 Not Found\n", time_str, ip, port);
            fclose(logf);
        }
        close(c_fd);
        return;
    }

    // 处理GET请求
    if (strcmp(parse.method, "GET") == 0) {
        send_file(c_fd, parse.path, parse.keep_alive);
        logf = fopen("server.log", "a");
        if (logf) {
            fprintf(logf, "[%s] %s:%d 响应文件: %s\n", time_str, ip, port, parse.path);
            fclose(logf);
        }
    } else {
        http_response(c_fd, "405 Method Not Allowed", "text/html", "<h1>405 Method Not Allowed</h1>");
        logf = fopen("server.log", "a");
        if (logf) {
            fprintf(logf, "[%s] %s:%d 响应 405 Method Not Allowed\n", time_str, ip, port);
            fclose(logf);
        }
    }

    // 处理keep-alive：若需要保持连接，重新添加到epoll
    if (parse.keep_alive) {
        struct epoll_event ev;
        ev.data.fd = c_fd;
        ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
        if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, c_fd, &ev) < 0) {
            perror("epoll_ctl add keep-alive fd failed");
            close(c_fd);
        }
    } else {
        close(c_fd);  // 关闭连接
    }
}