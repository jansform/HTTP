#include "setting.h"
#include "run.h"
#include "threadpool.h"

int main(int argc, char const *argv[])
{   
    //创建线程池
    ThreadPool *pool=threadpool_create(4,10);
    //创建epoll实例
    int ep_fd=epoll_create1(0);
    //创建套接字
    int s_fd=socket(AF_INET,SOCK_STREAM,0);

    //设置端口复用
    int opt = 1;
    if (setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return EXIT_FAILURE;
    }

    //监听文件描述符的事件类型
    struct epoll_event event;
    event.data.fd=s_fd;
    event.events=EPOLLIN;
    //加入实例
    epoll_ctl(ep_fd,EPOLL_CTL_ADD,s_fd,&event);
    //创建地址
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(9000);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    //绑定地址
    bind(s_fd,(struct sockaddr*)&addr,sizeof(addr));
    //监听
    listen(s_fd,1024);
    printf("服务器启动...\n");
    //
    struct epoll_event events[MAX_EVENTS];
    while(1){
        //获取就绪列表
        int ready=epoll_wait(ep_fd,events,MAX_EVENTS,-1);
        //处理事件
        //printf("就绪：%d\n",ready);
        for(size_t i=0;i<ready;i++){
            int fd=events[i].data.fd;
            if(fd==s_fd){
                //处理连接
                struct sockaddr_in c_addr;
                int len=sizeof(c_addr);
                int new_fd=accept(s_fd,(struct sockaddr*)&c_addr,&len);

                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&c_addr.sin_addr.s_addr,ip,INET_ADDRSTRLEN);
                printf("新连接 %d, %s:%d\n",new_fd,ip,ntohs(c_addr.sin_port));

                struct epoll_event ev;
                ev.data.fd=new_fd;
                ev.events=EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
                //添加客户端套接字
                epoll_ctl(ep_fd,EPOLL_CTL_ADD,new_fd,&ev);
                continue;
            }
            // 优先处理断开和错误事件
            // if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
            //     printf("断开 %d (EPOLL事件)\n",fd);
            //     epoll_ctl(ep_fd,EPOLL_CTL_DEL,fd,NULL);
            //     close(fd);
            //     continue;
            // }
            // 只处理纯EPOLLIN
            //if(events[i].events&EPOLLIN){
            else{
                int no_read=0;
                ioctl(fd,FIONREAD,&no_read);
                if(no_read>0){
                    int *client_fd=malloc(sizeof(int));
                    *client_fd=fd;
                    // pthread_t tid;
                    // pthread_create(&tid,NULL,run,client_fd);
                    // pthread_detach(tid);
                    threadpool_add_task(pool,run,client_fd);
                }
                perror("bad descriptor");
                //删除客户端套接字
                epoll_ctl(ep_fd,EPOLL_CTL_DEL,fd,NULL);
            }
        }
    }
    //关闭套接字
    close(s_fd);
    close(ep_fd);
    threadpool_destroy(pool);
    return 0;
}
