// #include "setting.h"
// #include "run.h"
// #include <libgen.h>

// int isvalid_path(char *path){
//     if(strstr(path,"../")||strstr(path,"..\\")||strstr(path,"./")||strstr(path,".\\")){
//         return 0;
//     }
//     return 1;
// }

// // 获取文件扩展名对应的MIME类型
// char *get_mime_type(char *path) {
//     const char *dot = strrchr(path, '.');
//     if (!dot) return "application/octet-stream";
    
//     if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) 
//         return "text/html";
//     if (strcmp(dot, ".txt") == 0) 
//         return "text/plain";
//     if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) 
//         return "image/jpeg";
//     if (strcmp(dot, ".png") == 0) 
//         return "image/png";
//     if (strcmp(dot, ".gif") == 0) 
//         return "image/gif";
//     if (strcmp(dot, ".css") == 0) 
//         return "text/css";
//     if (strcmp(dot, ".js") == 0) 
//         return "application/javascript";
//     if (strcmp(dot, ".json") == 0) 
//         return "application/json";
    
//     return "application/octet-stream";
// }

// //解析http请求
// void parse_http_request(char *req,HttpRequest *parse){
//     //解析第一行
//     sscanf(req, "%15s %1023s %15s",parse->method,parse->path,parse->version);

//     //解析请求头
//     char *host_start=strstr(req,"Host:");
//     if(host_start){
//         host_start+=5;
//         char *host_end=strchr(host_start,'\r');
//         if(host_end){
//             strncpy(parse->host,host_start,host_end-host_start);
//             parse->host[host_end-host_start]='\0';
//         }
//     }
// }

// void http_response(int sockfd,char *status,char *type,char *body){
//     //传递响应头
//     char headers[1024];
//     snprintf(headers,sizeof(headers),
//     "HTTP/1.1 200 %s\r\n"
//     "Content-Type: %s\r\n"
//     "Content-Length: %ld\r\n"
//     "Connection: keep-alive\r\n\r\n",
//     status,type,strlen(body));
//     send(sockfd,headers,strlen(headers),0);
//     send(sockfd,body,strlen(body),0);
// }

// void send_file(int sockfd,char *path){
//     // 默认页面处理
//     if (strcmp(path, "/") == 0) {
//         path = "/login.html";
//     }

//     // 安全路径检查
//     if (!isvalid_path(path)) {
//         http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
//         return;
//     }

//     // 构建完整路径
//     char full_path[512];
//     memmove(full_path, path+1, strlen(path));
//     //网站根目录
//     // char full_path[512]="index.html"; 
//     int fd=open(full_path,O_RDONLY);
//     //文件不存在时
//     if(fd<0){
//         if (errno == ENOENT) {
//             http_response(sockfd, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
//         } else if (errno == EACCES) {
//             http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
//         } else {
//             http_response(sockfd, "500 Internal Server Error", "text/html", "<h1>500 Internal Server Error</h1>");
//         }
//         return;
//     }
//     //获取文件信息
//     struct stat st;
//     fstat(fd,&st);
    
//     //发送http头
//     char headers[1024];
//     snprintf(headers,sizeof(headers),
//     "HTTP/1.1 200 OK\r\n"
//     "Content-Type: %s\r\n"
//     "Content-Length: %ld\r\n"
//     "Connection: keep-alive\r\n\r\n",
//     get_mime_type(full_path),st.st_size);
//     send(sockfd,headers,strlen(headers),0);

    
//     // char buf[BUFFER_SIZE];
//     // size_t len;
//     // while((len=read(fd,buf,sizeof(buf)))>0){
//     //     send(sockfd,buf,len,0);
//     // }
//     //使用sendfile发送文件内容
//     ssize_t len;
//     off_t offset=0;
//     len=sendfile(sockfd,fd,&offset,st.st_size);
//     if(len<0){
//         printf("发送失败\n");
//     }
//     close(fd);
// }

// void run(void *arg){
//     int c_fd=*(int*)arg;
//     free(arg);
//     char req[BUFFER_SIZE];
//     ssize_t len=recv(c_fd,req,sizeof(req)-1,0);
//     if(len<=0){
//         close(c_fd);
//         return;
//     }
//     req[len]='\0';
//     HttpRequest parse={0};
//     parse_http_request(req,&parse);
//     //printf("[%s] %s %s\n",parse.host,parse.method,parse.path);

//     // 检查是否为favicon.ico请求
//     if(strcmp(parse.path, "/favicon.ico") == 0){
//         http_response(c_fd, "404 Not Found", "text/html", "");
//         close(c_fd);
//         return;
//     }

//     if(strcmp(parse.method,"GET")==0){
//         send_file(c_fd,parse.path);
//     }else{
//         http_response(c_fd,"405 Method Not Allowed","text/html","<h1>405 Method Not Allowed</h1>");
//     }

//     if(strstr(req,"Connection: close")){
//         close(c_fd);
//         return;
//     }
//     close(c_fd);
//     return;
// }

#include "setting.h"
#include "run.h"
#include <libgen.h>  
#include <ctype.h>  // 添加ctype.h用于字符处理

// URL解码函数
void url_decode(char *dst, const char *src) {
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            // 取出两个十六进制字符，转为数字
            char hex[3] = {src[1], src[2], '\0'};
            /*把 hex 里的两位十六进制字符串（如 "20"）转成十进制整数（32），
            再强制转换为 char 类型，赋值给 *dst。*/
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

int isvalid_path(char *path){
    if(strstr(path,"../")||strstr(path,"..\\")||strstr(path,"./")||strstr(path,".\\")){
        return 0;
    }
    return 1;
}

// 获取文件扩展名对应的MIME类型
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

//解析http请求
void parse_http_request(char *req, HttpRequest *parse){
    //解析第一行
    char raw_path[1024];
    sscanf(req, "%15s %1023s %15s", parse->method, raw_path, parse->version);
    
    // URL解码路径
    url_decode(parse->path, raw_path);

    //解析请求头
    char *host_start = strstr(req, "Host:");
    if(host_start){
        host_start += 5;
        char *host_end = strchr(host_start, '\r');
        if(host_end){
            strncpy(parse->host, host_start, host_end - host_start);
            parse->host[host_end - host_start] = '\0';
        }
    }
}

void http_response(int sockfd, char *status, char *type, char *body){
    //传递响应头
    char headers[1024];
    snprintf(headers, sizeof(headers),
    "HTTP/1.1 200 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: keep-alive\r\n\r\n",
    status, type, strlen(body));
    send(sockfd, headers, strlen(headers), 0);
    send(sockfd, body, strlen(body), 0);
}

void send_file(int sockfd, char *path){
    // 默认页面处理
    if (strcmp(path, "/") == 0) {
        path = "/login.html";
    }

    // 安全路径检查
    if (!isvalid_path(path)) {
        http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
        return;
    }

    // 构建完整路径
    char full_path[512];
    memmove(full_path, path + 1, strlen(path));
    
    int fd = open(full_path, O_RDONLY);
    //文件不存在时
    if(fd < 0){
        if (errno == ENOENT) {
            http_response(sockfd, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
        } else if (errno == EACCES) {
            http_response(sockfd, "403 Forbidden", "text/html", "<h1>403 Forbidden</h1>");
        } else {
            http_response(sockfd, "500 Internal Server Error", "text/html", "<h1>500 Internal Server Error</h1>");
        }
        return;
    }
    
    //获取文件信息
    struct stat st;
    fstat(fd, &st);
    
    //发送http头
    char headers[1024];
    snprintf(headers, sizeof(headers),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %ld\r\n"
    "Connection: keep-alive\r\n\r\n",
    get_mime_type(full_path), st.st_size);
    send(sockfd, headers, strlen(headers), 0);

    // 循环使用sendfile发送超大文件内容
    ssize_t sent = 0;
    off_t offset = 0;
    while (sent < st.st_size) {
        ssize_t n = sendfile(sockfd, fd, &offset, st.st_size - sent);
        if (n <= 0) {
            if (errno == EINTR) continue; // 被信号中断，重试
            printf("发送失败\n");
            break;
        }
        sent += n;
    }
    close(fd);
}

void run(void *arg){
    // 获取客户端文件描述符
    int c_fd = *(int*)arg;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(c_fd, (struct sockaddr*)&addr, &addr_len);

    free(arg);
    char req[BUFFER_SIZE];
    ssize_t len = recv(c_fd, req, sizeof(req) - 1, 0);
    if(len <= 0){
        close(c_fd);
        return;
    }
    req[len] = '\0';
    HttpRequest parse = {0};
    parse_http_request(req, &parse);

    // 获取当前时间字符串
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    // 获取客户端IP和端口，优先从X-Forwarded-For头部获取
    // 代理服务器发送的请求头中包含X-Forwarded-For字段，记录客户端的真实IP
    char ip[INET_ADDRSTRLEN] = {0};
    int port = ntohs(addr.sin_port);
    char *xff = strstr(req, "X-Forwarded-For:");
    if (xff) {
        xff += strlen("X-Forwarded-For:");
        while (*xff == ' ') xff++;
        char *end = strchr(xff, '\r');
        if (end && end - xff < sizeof(ip)) {
            strncpy(ip, xff, end - xff);
            ip[end - xff] = '\0';
        }
    }
    if (ip[0] == 0) {
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    }

    // 日志文件写入
    FILE *logf = fopen("server.log", "a");
    if (logf) {
        fprintf(logf, "[%s] %s:%d 请求 %s %s\n", time_str, ip, port, parse.method, parse.path);
        fclose(logf);
    }

    // 检查是否为favicon.ico请求
    if(strcmp(parse.path, "/favicon.ico") == 0){
        http_response(c_fd, "404 Not Found", "text/html", "");
        // 记录响应
        logf = fopen("server.log", "a");
        if (logf) {
            fprintf(logf, "[%s] %s:%d 响应 404 Not Found\n", time_str, ip, port);
            fclose(logf);
        }
        close(c_fd);
        return;
    }

    if(strcmp(parse.method, "GET") == 0){
        send_file(c_fd, parse.path);
        // 记录响应的文件名
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

    if(strstr(req, "Connection: close")){
        close(c_fd);
        return;
    }
    close(c_fd);
    return;
}