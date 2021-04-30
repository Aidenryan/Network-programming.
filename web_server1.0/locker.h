//三种线程同步机制
#pragma once
#include <exception>
#include <pthread.h>
#include <semaphore.h>

//封装信号量的类
class sem
{
private:
    sem_t m_sem;
public:
    //创建并初始化信号量
    sem()
    {
        if(sem_init(&m_sem, 0, 0) != 0)
        {
            //构造函数没有返回值，可以直接抛出异常来报告错误
            throw std::exception();
        }
    }
    //销毁信号量
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    //等待信号量
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    //增加信号量
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
};

//封装互斥锁的类
class locker
{
private:
    pthread_mutex_t m_mutex;
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    //获取互斥锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    //释放互斥锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
};

//封装条件变量
class cond
{
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
public:
    //创建并初始化条件变量
    cond()
    {
        if(pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }

        if(pthread_cond_init(&m_cond, NULL) != 0)
        {
            //构造函数一旦出问题，就立刻释放已经分配的资源
            pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    //等待条件变量
    bool wait()
    {
        int ret = 0;
        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
};






