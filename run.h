#ifndef RUN_H
#define RUN_H

typedef struct{
    char method[256];   //GET/POST
    char path[256];     //index.html
    char version[256];  //HTTP/1.1
    char host[256];     //localhost
}HttpRequest;

void url_decode(char *dst, const char *src);

int isvalid_path(char *path);

char *get_mime_type(char *path);

void parse_http_request(char *request, HttpRequest *parse);

void http_response(int sockfd,char *status,char *type,char *body);

void send_file(int sockfd,char *path);

void run(void *arg);


#endif 