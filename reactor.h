#ifndef _REACTOR_H_
#define _REACTOR_H_

#define BUFFER_LENGTH 4096
#define MAX_EPOLL_EVENT 1024*1024

typedef int CALLBACK(int fd,void* arg);
typedef int HANDLER(void *arg);
struct event
{
    int fd;
    int events;  //EPOLLIN  EPOLLOUT
    void *arg;
    int (*callback)(int fd,void* arg);

    int status;
    
    char rbuffer[BUFFER_LENGTH];
    int rlength;
    char sbuffer[BUFFER_LENGTH];
    int slength;

    long last_active;

    
};

struct reactor
{
    int epfd;
    struct event *events;
    HANDLER *hander;
};

int callback_accept(int fd,void *arg);
int callback_recv(int fd,void *arg);
int callback_send(int fd,void *arg);
int event_set(struct event* ev,int fd,CALLBACK *callback,void* arg);
int event_add(int epfd,int event_type,struct event* ev);
int event_del(int epfd,struct event* ev);
int init_socket(short port);
int reactor_addlistener(struct reactor* r,int fd,CALLBACK *callback);
int reactor_init(struct reactor *r);
void reactor_run(struct reactor* r);
int reactor_destory(struct reactor* r);
int reactor_setup();


#endif 