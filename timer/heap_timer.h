#ifndef _TIME_HEAP_H
#define _TIME_HEAP_H
#include<iostream>
#include<stdio.h>
#include<vector>
#include<netinet/in.h>
#include<time.h>
using namespace std;

class util_timer; //前向声明

//用户的信息
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;  //指向一个定时器
};

//定时器结点
class util_timer
{
    public:
		util_timer(){}
	public:
		time_t expire;	//任务超时的时间，这里使用绝对时间
		void (*cb_func) (client_data*); //任务回调函数，回调函数处理客户数据
		client_data* user_data;	//交叉使用
};

class time_heap
{
    private:
        vector<util_timer*> heap; //堆数组
        int capacity;
        int cur_size;
        void down(int node);
        bool empty(){return cur_size == 0;}
        void pop();
        util_timer* top();
    public:
        time_heap():capacity(0),cur_size(0){}
        ~time_heap(){}
        void add_timer(util_timer *timer);
        void adjust_timer(util_timer *timer);
        void del_timer(util_timer *timer);
        void tick();
};


class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，选择开启EPOLLONESHOT， ET模式
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    time_heap m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif