/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-31
 *
--*/
#ifndef __POOLOBJECT_HPP__
#define __POOLOBJECT_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>

template<typename SingletonT>
class ThreadPoolObject
{
public:
    static void ObjectFree(void* buffer)
    {
        SingletonT* pObject = (SingletonT*)buffer;
        delete pObject;
    }

    static void ObjectKeyInit()
    {
        pthread_key_create(&ThreadPoolObject<SingletonT>::objectKey, &ThreadPoolObject<SingletonT>::ObjectFree);
    }

    static inline SingletonT* GetObject()
    {
        pthread_once(&ThreadPoolObject<SingletonT>::objectOnce, &ThreadPoolObject<SingletonT>::ObjectKeyInit);
        return (SingletonT*)pthread_getspecific(ThreadPoolObject<SingletonT>::objectKey);
    }

    static SingletonT& Instance()
    {
        SingletonT* pObject = ThreadPoolObject<SingletonT>::GetObject();
        if(!pObject)
        {
            pObject = new SingletonT();
            pthread_setspecific(ThreadPoolObject<SingletonT>::objectKey, pObject);
        }
        return *pObject;
    }

private:
    static pthread_once_t objectOnce;
    static pthread_key_t objectKey;
};

template<typename SingletonT>
pthread_once_t ThreadPoolObject<SingletonT>::objectOnce = PTHREAD_ONCE_INIT;

template<typename SingletonT>
pthread_key_t ThreadPoolObject<SingletonT>::objectKey;

template<typename SingletonT>
class ProcessPoolObject
{
public:
    static SingletonT& Instance()
    {
        static SingletonT instance;
        return instance;
    }
};

#if defined(POOL_USE_THREADPOOL)
    #define PoolObject ThreadPoolObject
#else
    #define PoolObject ProcessPoolObject
#endif

#endif // define __POOLOBJECT_HPP__
