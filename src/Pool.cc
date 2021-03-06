/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "Pool.hpp"

/////////////////////////////////////////////////////////////////////////////////////////
// Process Pool
ProcessPool& ProcessPool::Instance()
{
    static ProcessPool instance;
    return instance;
}

ProcessPool::ProcessPool() :
    m_bStartup(false),
    m_id(0),
    m_IdleTimeout(-1)
{
}

int ProcessPool::Startup(uint32_t num)
{
    for(uint32_t i = 1; i < num; ++i)
    {
        pid_t pid = fork();
        if(pid == -1)
            return -1;
        else if(pid == 0)
        {
            m_id = i;
            break;
        }
    }

    EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
    if(scheduler.CreateScheduler() == -1)
        return -1;

    scheduler.SetIdleTimeout(m_IdleTimeout);

    m_bStartup = true;
    for(std::list<boost::function<bool(void)> >::iterator iter = m_StartupCallbackList.begin();
        iter != m_StartupCallbackList.end();
        ++iter)
    {
        if(!(*iter)())
            return -1;
    }

    scheduler.Dispatch();
    return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Thread Pool
ThreadPool& ThreadPool::Instance()
{
    static ThreadPool instance;
    return instance;
}

int ThreadPool::Startup(uint32_t num)
{
    m_bStartup = true;

    for(uint32_t i = 1; i < num; ++i)
    {
        pthread_t tid;
        if(0 != pthread_create(&tid, NULL, ThreadPool::ThreadProc, (void*)i))
            return -1;
    }

    ThreadPool::ThreadProc(0);
    return 0;
}

pthread_once_t pool_id_once = PTHREAD_ONCE_INIT;
pthread_key_t pool_id_key;

void pool_id_free(void* buffer)
{
    free(buffer);
}

void pool_id_init()
{
    pthread_key_create(&pool_id_key, &pool_id_free);
}

void* ThreadPool::ThreadProc(void* paramenter)
{
    pthread_once(&pool_id_once, &pool_id_init);
    uint32_t* pID = (uint32_t*)pthread_getspecific(pool_id_key);
    if(!pID)
    {
        pID = (uint32_t*)malloc(sizeof(uint32_t));
        pthread_setspecific(pool_id_key, pID);
    }
    *pID = static_cast<uint32_t>(reinterpret_cast<long>(paramenter));

    EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
    if(scheduler.CreateScheduler() == -1)
        return NULL;

    scheduler.SetIdleTimeout(ThreadPool::Instance().m_IdleTimeout);

    std::list<boost::function<bool(void)> >& list = ThreadPool::Instance().m_StartupCallbackList;
    for(std::list<boost::function<bool(void)> >::iterator iter = list.begin();
        iter != list.end();
        ++iter)
    {
        if(!(*iter)())
            return NULL;
    }

    scheduler.Dispatch();
    return NULL;
}

uint32_t ThreadPool::GetID()
{
    pthread_once(&pool_id_once, &pool_id_init);

    uint32_t* pID = (uint32_t*)pthread_getspecific(pool_id_key);
    if(!pID)
    {
        pID = (uint32_t*)malloc(sizeof(uint32_t));
        pthread_setspecific(pool_id_key, pID);
        *pID = 0;
    }
    return *pID;
}

ThreadPool::ThreadPool() :
    m_bStartup(false),
    m_IdleTimeout(-1)
{
}










