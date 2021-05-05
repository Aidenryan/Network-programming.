/*
半同步/半反应堆线程池
*/
#pragma once

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

//线程池类  T是任务类
template<typename T>
class threadpool
{
private:
    //静态成员函数与普通成员函数的根本区别在于：普通成员函数有 this 指针，可以访问类中的任意成员；
    //而静态成员函数没有 this 指针，只能访问静态成员（包括静态成员变量和静态成员函数）
    static void* worker(void* arg);//工作线程运行的函数，它不断从工作队列取出任务并执行
    void run();

private:
    int m_thread_number; //线程池中线程数
    int m_max_requests; //请求队列中允许的最大请求数
    pthread_t* m_threads; //描述线程池的数组，大小为m_thread_number
    std::list<T*> m_workQueue; //请求队列
    locker m_queueLocker; //保护请求队列的互斥锁住
    sem m_queueStat; // 是否有任务需要处理
    bool m_stop; //是否结束线程

public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();

    //往请求队列添加任务
    bool append(T* request);
};

template<typename T>//创建线程池类的时候就开始创建线程
threadpool<T>::threadpool(int thread_number, int max_requests):
        m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL)
{
    if((thread_number <= 0) || (max_requests <= 0))
    {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];

    if(!m_threads)
    {
        throw std::exception();
    }

    //创建thread_number个线程，并将它们设置为脱离线程
    for(int i(0); i < thread_number; ++i)
    {
        printf("Create the %dth thred\n", i);

        if(pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete [] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i]))
        {
            delete [] m_threads;
            throw std::exception();
        }
    }

}

template<typename T>
threadpool<T>::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request)
{
    //操作工作队列一定需要加锁
    m_queueLocker.lock();
    if(m_workQueue.size() > m_max_requests)
    {
        m_queueLocker.unlock();
        return false;
    }

    m_workQueue.push_back(request);
    m_queueLocker.unlock();
    m_queueStat.post();// +1 唤醒wait
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(!m_stop)
    {
        m_queueStat.wait();//限制当任务为零时候阻塞 -1
        m_queueLocker.lock();
        if(m_workQueue.empty())
        {
            m_queueLocker.unlock();
            continue;
        }

        T* req = m_workQueue.front();
        m_workQueue.pop_front();
        m_queueLocker.unlock();

        if(!req)
        {
            continue;
        }

        req->process();
    }
}


