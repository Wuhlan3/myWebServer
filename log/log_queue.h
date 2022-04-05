#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <queue>
#include "../lock/locker.h"
using namespace std;
/*
使用C++的queue来实现阻塞队列
*/
template <class T>
class block_queue
{
public:
    block_queue(int maxsize = 1000){
        if(maxsize < 0){exception();}
        m_size = 0;
        m_maxsize = maxsize;
    }
    ~block_queue(){}
    bool is_full(){
        m_mutex.lock();
        if(q.size() == m_maxsize){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool is_empty(){
        m_mutex.lock();
        if(q.size() == 0){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool get_front(T &value){
        m_mutex.lock();
        if(q.size() == 0){
            m_mutex.unlock();
            return false;
        }
        value = q.front();
        m_mutex.unlock();
        return true;
    }
    int get_size(){
        m_mutex.lock();
        int ret = q.size();
        m_mutex.unlock();
        return ret;
    }
    int get_maxsize(){
        return m_maxsize;
    }
    bool push(const T &value){
        m_mutex.lock();
        if (m_size >= m_maxsize)
        {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }

        q.push(value);

        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    bool pop(T &value){
        m_mutex.lock();
        //如果没有任何消息，则线程阻塞在这里
        while (m_size <= 0)
        {
            
            if (!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }

        value = q.front();
        q.pop();
        m_mutex.unlock();
        return true;
    }
    bool pop(T &value, int timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if (m_size <= 0)
        {
            t.tv_sec = now.tv_sec + timeout / 1000;
            t.tv_nsec = (timeout % 1000) * 1000;
            if (!m_cond.timewait(m_mutex.get(), t))
            {
                m_mutex.unlock();
                return false;
            }
        }

        if (m_size <= 0)
        {
            m_mutex.unlock();
            return false;
        }

        value = q.front();
        q.pop();

        m_mutex.unlock();
        return true;
    }
private:
    locker m_mutex;
    cond m_cond;
    queue<T> q;
    
    int m_size;
    int m_maxsize;
};

#endif