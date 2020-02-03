#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>



#include <fcntl.h>
#include <unistd.h> 
#include <errno.h> 

#include "reactor.h"


#define BUFFER_LENGTH 4096
#define MAX_EPOLL_EVENT 1024*1024

int callback_accept(int listen_fd,void *arg)
{
    struct reactor *r = (struct reactor*)arg;

    struct socketaddr_in client_addr;
    socket_t len = sizeof(client_addr);

    int client_fd;
    if((client_fd = accept(listen_fd,(struct sockaddr*)&)client_addr,&len)) == -1)
    {
        if(errno == EAGAIN ||  errno == EINTR)
        {
            return 1;
        }
        printf("accept: %s\n", strerror(errno));
		return -1;
    }

    int i = 0;
    do
    {
        for(i=0;i<MAX_EPOLL_EVENT;i++)
        {
            if(reactor->event[i].status == 0)
                break;
        }
        if(i==MAX_EPOLL_EVENT)
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EPOLL_EVENT);
            break;
        }

        if(fcntl(client_fd,F_SETFL,O_NONBLOCK)<0)
        {
            printf("%s: fcntl nonblocking failed, %d\n", __func__, MAX_EPOLL_EVENT);
			break;
        }

        event_set(&reactor->events[clientfd],client_fd,callback_recv,r);
        event_add(reactor->epfd,EPOLLIN,&reactor->events[client_fd]);
    }while(0);

    printf("new connect [%s:%d][time:%ld],pos[%d]\n",
        inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),reactor->events[i].last_active,i));

    return 0;
}
int callback_recv(int fd,void *arg)
{
    struct reactor* r = (struct reactor*)arg;
    struct event *ev = r->events+fd;

    int len = recv(fd,ev->buffer,BUFFER_LENGTH,0);
    event_del(r->epfd,ev);

    if(len >0)
    {
        ev->rlength = len;
        ev->rbuffer[len] = '\0';

        printf("C[%d]:%s\n",fd,ev->rbuffer);

        //r->handler(ev);  //处理业务逻辑

        event_set(ev,fd,callback_send,r);
        event_add(r->epfd,EPOLLOUT,ev);

    }else if(len == 0)
    {
        close(ev->fd);
        printf("[fd=%d] pos[%ld], closed\n", fd, ev-reactor->events);
    }
    else
    {
        close(ev->fd);
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }

    return len;
}
int callback_send(int fd,void *arg)
{
    struct reactor *r = (struct reactor*)arg;
    struct event *ev = r->events+fd;

    int len = send(fd,ev->sbuffer,ev->slength,0);
    if(len < 0)
    {
        printf("send[fd=%d], [%d]%s\n", fd, len, ev->sbuffer);

        event_del(r->epfd,ev);
        event_set(ev,fd,callback_recv,r);
        event_add(r->epfd,EPOLLIN,ev);
    }else 
    {
        close(ev->fd);

        event_del(r->epfd,ev);
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }
    return len;
}

int event_set(struct event* ev,int fd,CALLBACK *callback,void* arg)
{
    ev->fd = fd;
    ev->callback = callback;
    ev->arg = arg;
    ev->last_active = time(NULL);

    return 0;

}
int event_add(int epfd,int event_type,struct event* ev)
{
    //epoll define as bellow 
    /*struct epoll_event
    {
      uint32_t events;   //Epoll events 
      epoll_data_t data;    //User data variable 
    } __attribute__ ((__packed__));

    typedef union epoll_data
    {
      void *ptr;
      int fd;
      uint32_t u32;
      uint64_t u64;
    } poll_data_t; */

    struct epoll_event ep_event = {0,{0}};
    ep_event.data.ptr = ev;
    ep_event.event = ev->events = event_type;

    int op;
    if(ev->status == 1)
    {
        op = EPOLL_CTL_MOD;
    }else
    {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if(epoll_ctl(epfd,op,fd,&ep_event) <0)
    {
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
		return -1;
    }

    return 0;

}
int event_del(int epfd,struct event* ev)
{
    struct epoll_event ep_ev = {0, {0}};

    if(ev->status !=1)
    {
        return -1;
    }

    ep_ev.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(epfd,EPOLL_CTL_DEL,ev->fd,&ep_ev);

    return 0;
}
int init_socket(short port)
{

    int fd = socket(AF_INET,SOCK_STREAM,0);
    fcntl(fd,F_SETFL,O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

     if(-1 == bind(fd,(struct sockaddr*)server_addr),sizeof(server_addr))
     {
         printf("bind failed : %s \n ",strerror(errno));
     }

     if(listen(fd,20) < 0)
     {
         printf("listen failed: %s \n ",strerror(errno));
     }

    return fd;
}

int reactor_addlistener(struct reactor* r,int fd,CALLBACK *callback)
{
    if( r==NULL || r->events==NULL )
        return -1;

    event_set(&r->events[fd],fd,callback,(void*)r); 
    event_add(r->epfd,EPOLIN,&r->events[fd]);
    return 0;
}

int reactor_init(struct reactor *r)
{
    if(!r)
        return -1;
    memset(r,0,sizeof(struct reactor));

    r->ep_fd = epoll_create(1);
    if(r->ep_fd)     
    {
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
        return -2;
    }   

    r->events = new event[MAX_EPOLL_EVENT];
    if(r->evnets == NULL)
    {
        printf("create epfd in %s err %s\n", __func__, strerror(errno));
		close(reactor->epfd);
		return -3;
    }

    return 0;
}
void reactor_run(struct reactor* r)
{
    struct epoll_event events[MAX_EPOLL_EVENT+1];
    int nready = epoll_wait(r->epfd,events,MAX_EPOLL_EVENT,1000)
    if(nready < 0)
    {
        printf("epollwait error!\n");
        continue;
    }

    for(int i=0;i<nready;i++)
    {
        struct event *ev = (struct event*)events[i].data.ptr;

        if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
        {
            ev->callback(ev->fd,ev->arg);
        }
        if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
        {
            ev->callback(ev->fd,ev->arg);
        }
    }
        
}
int reactor_destory(struct reactor* r)
{
    close(r->epfd);
    free(r->events);
}

int reactor_setup()
{
    // 1.init socket
    unsigned short port = SERVER_PORT;
    int sockfd = init_socket(port);

    struct reactor* m_reactor = new reactor;

    // 2.init reactor 
    reacotr_init(m_reacotr);

    // 3.add listener into epoll
    reactor_addlistener(m_reactor,sockfd,callback_accept);

    reactor_run();

    reactor_destory();
    close(sockfd);

    return 0;

}

int main()
{
    reactor_setup();
    return 0;

}

