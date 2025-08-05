#ifndef RUN_H
#define RUN_H

typedef struct{
    char method[256];
    char path[256];
    char version[256];
    char host[256];
    int keep_alive;  // 新增：是否保持连接
} HttpRequest;

void url_decode(char *dst, const char *src);

int isvalid_path(char *path);

char *get_mime_type(char *path);

void parse_http_request(char *request, HttpRequest *parse);

void http_response(int sockfd,char *status,char *type,char *body);

void send_file(int sockfd, char *path, int keep_alive);

void run(void *arg);


#endif 