#include "heap_timer.h"
#include "../http/http_conn.h"
//堆的下滤过程
void time_heap::down(int node)
{
    util_timer* tmp = heap[node];
    int child;
    for(;(node*2+1) < cur_size;node = child)
    {
        child = node*2+1;
        if((child+1 < cur_size) && (heap[child+1]->expire < heap[child]->expire))
        {
            child++;
        }
        if(heap[child]->expire < tmp->expire)
        {
            heap[node] = heap[child];
        }
        else break;
    }
    heap[node] = tmp;
}
void time_heap::pop()
{
    if(empty())
    {
        throw std::exception();
    }
    heap[0] = heap[cur_size-1];
    cur_size--;
    down(0);
}
util_timer* time_heap::top()
{
    if(empty())
        throw std::exception();
    return heap[0];
}

void time_heap::add_timer(util_timer *timer)
{
    if(timer == nullptr)return;
    
    int node = cur_size ++;
    if(cur_size > capacity){
        heap.emplace_back(timer);
        capacity++;
    }

    int parent;
    for(;node > 0; node = parent){
        parent = (node-1)/2;
        if(timer->expire >= heap[parent]->expire)
            break;
        else heap[node] = heap[parent];
    }
    heap[node] = timer;
}
void time_heap::adjust_timer(util_timer *timer)
{
    if(timer == NULL)
        return;
    down(0);
}
void time_heap::del_timer(util_timer *timer)
{
    if(timer == NULL)
        return;
    /*将定时器对应的回调函数指向空，这样起到延迟删除的作用，节省了真正删除定时器的开销，但是会容易使堆数组膨胀*/
    timer->cb_func = NULL;
}
void time_heap::tick()
{
    time_t timeout = time(NULL);
    while(!empty())
    {
        /*如果堆顶没到期则退出循环*/
        if(heap[0]->expire > timeout)
            break;
        /*否则执行堆顶定时器中的任务*/
        if(heap[0]->cb_func != NULL)
            heap[0]->cb_func(heap[0]->user_data);
        /*将堆顶删除*/
        pop();
    }
}

//初始化，超时时间
void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);  //获取旧的描述符选项
    int new_option = old_option | O_NONBLOCK;   
    fcntl(fd, F_SETFL, new_option);       //将其设置为非阻塞
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//触发SIGALRM后信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    //把信号发送到一个管道去
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//注册信号捕捉，进行回调
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
