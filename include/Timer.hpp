/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <sys/time.h>
#include <utility>
#include <vector>
#include <list>
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>

#ifndef TIMER_DEFAULT_INTERVAL
    // default check interval 10ms
    #define TIMER_DEFAULT_INTERVAL 10
#endif

template<typename DataT, typename IdT, typename TimeValueT>
struct TimerItem
{
    TimerItem<DataT, IdT, TimeValueT>* PrevItem;
    TimerItem<DataT, IdT, TimeValueT>* NextItem;
    TimerItem<DataT, IdT, TimeValueT>** Base;

    IdT TimerId;
    TimeValueT Timeval;
    boost::function<void(DataT)> Callback;

    DataT Data;

    TimerItem() :
        PrevItem(NULL),
        NextItem(NULL),
        Base(NULL),
        TimerId(0),
        Timeval(0)
    {
    }

    inline void Invoke()
    {
        Callback(Data);
    }
};
template<typename IdT, typename TimeValueT>
struct TimerItem<void, IdT, TimeValueT>
{
    TimerItem<void, IdT, TimeValueT>* PrevItem;
    TimerItem<void, IdT, TimeValueT>* NextItem;
    TimerItem<void, IdT, TimeValueT>** Base;

    IdT TimerId;
    TimeValueT Timeval;
    boost::function<void()> Callback;

    TimerItem() :
        PrevItem(NULL),
        NextItem(NULL),
        Base(NULL),
        TimerId(0),
        Timeval(0)
    {
    }

    inline void Invoke()
    {
        Callback();
    }
};

template<typename TimerBaseT, int32_t Interval>
class TimerStartup :
    public boost::noncopyable
{
public:
    TimerStartup()
    {
        if(Pool::Instance().IsStartup())
            Startup();
        else
            Pool::Instance().RegisterStartupCallback(boost::bind(&TimerStartup<TimerBaseT, Interval>::Startup, this), true);
    }

    virtual ~TimerStartup()
    {
    }

    bool Startup()
    {
        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();

        int timeout = scheduler.GetIdleTimeout();
        if(timeout == -1 || timeout > Interval)
            scheduler.SetIdleTimeout(Interval);

        scheduler.RegisterLoopCallback(boost::bind(&TimerBaseT::CheckTimer, reinterpret_cast<TimerBaseT*>(this)));
        return true;
    }
};

template<typename DataT, typename IdT, typename TimeValueT, int32_t Interval>
class TimerBase :
    public TimerStartup<TimerBase<DataT, IdT, TimeValueT, Interval>, Interval>
{
protected:
    IdT m_LastTimerId;
    TimeValueT m_LastTimeval;

#define TVN_BITS    6
#define TVR_BITS    8
#define TVN_SIZE    (1 << TVN_BITS)
#define TVR_SIZE    (1 << TVR_BITS)
#define TVN_MASK    (TVN_SIZE - 1)
#define TVR_MASK    (TVR_SIZE - 1)
#define MAX_TVAL    ((1ULL << (TVR_BITS + 4*TVN_BITS)) - 1)

    TimerItem<DataT, IdT, TimeValueT>* m_Vector1[TVR_SIZE];
    TimerItem<DataT, IdT, TimeValueT>* m_Vector2[TVN_SIZE];
    TimerItem<DataT, IdT, TimeValueT>* m_Vector3[TVN_SIZE];
    TimerItem<DataT, IdT, TimeValueT>* m_Vector4[TVN_SIZE];
    TimerItem<DataT, IdT, TimeValueT>* m_Vector5[TVN_SIZE];

    boost::unordered_map<IdT, TimerItem<DataT, IdT, TimeValueT>*> m_TimerDataMap;

    TimeValueT GetTimeval()
    {
        timeval tv;
        gettimeofday(&tv, NULL);
        TimeValueT val = (tv.tv_sec * 1000 + tv.tv_usec / 1000) / Interval;
        return val;
    }

    int InternalAddTimerItem(TimerItem<DataT, IdT, TimeValueT>* pItem)
    {
        TimeValueT idx = pItem->Timeval - m_LastTimeval;
        TimerItem<DataT, IdT, TimeValueT>** ppVector = NULL;

        if(pItem->Timeval < m_LastTimeval)
        {
            // already timeout item.
            ppVector = &m_Vector1[m_LastTimeval & TVR_MASK];
        }
        else if(idx < TVR_SIZE)
        {
            ppVector = &m_Vector1[pItem->Timeval & TVR_MASK];
        }
        else if(idx < (1 << (TVR_BITS + TVN_BITS)))
        {
            ppVector = &m_Vector2[(pItem->Timeval >> TVR_BITS) & TVN_MASK];
        }
        else if(idx < (1 << (TVR_BITS + 2*TVN_BITS)))
        {
            ppVector = &m_Vector3[(pItem->Timeval >> (TVR_BITS + TVN_BITS)) & TVN_MASK];
        }
        else if(idx < (1 << (TVR_BITS + 3*TVN_BITS)))
        {
            ppVector = &m_Vector4[(pItem->Timeval >> (TVR_BITS + 2*TVN_BITS)) & TVN_MASK];
        }
        else
        {
            TimeValueT tv = pItem->Timeval;
            if(idx > MAX_TVAL)
                tv = MAX_TVAL + m_LastTimeval;
            ppVector = &m_Vector5[(tv >> (TVR_BITS + 3*TVN_BITS)) & TVN_MASK];
        }

        pItem->Base = ppVector;

        if(ppVector[0] == NULL)
            ppVector[0] = pItem;
        else
        {
            pItem->NextItem = ppVector[0];
            ppVector[0]->PrevItem = pItem;
            ppVector[0] = pItem;
        }
        return 0;
    }

    bool cascade(TimerItem<DataT, IdT, TimeValueT>** pVector, TimeValueT index)
    {
        TimerItem<DataT, IdT, TimeValueT>* pTimerItem = pVector[index];
        pVector[index] = NULL;
        while(pTimerItem)
        {
            TimerItem<DataT, IdT, TimeValueT>* pNextItem = pTimerItem->NextItem;

            pTimerItem->PrevItem = NULL;
            pTimerItem->NextItem = NULL;
            pTimerItem->Base = NULL;
            InternalAddTimerItem(pTimerItem);

            pTimerItem = pNextItem;
        }
        return (index != 0);
    }

    void PrintVector(TimerItem<DataT, IdT, TimeValueT>** pVector, int size)
    {
        for(int i=0; i<size; ++i)
        {
            TimerItem<DataT, IdT, TimeValueT>* pItem = pVector[i];
            printf("  %03d ", i);
            if(pItem)
            {
                bool b = true;
                while(pItem)
                {
                    TimerItem<DataT, IdT, TimeValueT>* pNext = pItem->NextItem;
                    if(b)
                    {
                        printf("timeval: (0x%016lX)%lu\n", pItem->Timeval, pItem->Timeval);
                        b = false;
                    }
                    else
                        printf("      timeval: (0x%016lX)%lu\n", pItem->Timeval, pItem->Timeval);

                    pItem = pNext;
                }
            }
            else
                printf("timeval: NULL\n");
        }
    }

public:
    TimerBase() :
        m_LastTimerId(0)
    {
        m_LastTimeval = GetTimeval();

        bzero(&m_Vector1, sizeof(TimerItem<DataT, IdT, TimeValueT>*)*TVR_SIZE);
        bzero(&m_Vector2, sizeof(TimerItem<DataT, IdT, TimeValueT>*)*TVN_SIZE);
        bzero(&m_Vector3, sizeof(TimerItem<DataT, IdT, TimeValueT>*)*TVN_SIZE);
        bzero(&m_Vector4, sizeof(TimerItem<DataT, IdT, TimeValueT>*)*TVN_SIZE);
        bzero(&m_Vector5, sizeof(TimerItem<DataT, IdT, TimeValueT>*)*TVN_SIZE);
    }

    void Update(IdT timerId, int timeout)
    {
        typename boost::unordered_map<IdT, TimerItem<DataT, IdT, TimeValueT>*>::iterator iter = m_TimerDataMap.find(timerId);
        if(iter == m_TimerDataMap.end())
            return;

        TimerItem<DataT, IdT, TimeValueT>* pItem = iter->second;
        if(pItem->NextItem)
            pItem->NextItem->PrevItem = pItem->PrevItem;

        if(pItem->PrevItem)
            pItem->PrevItem->NextItem = pItem->NextItem;
        else
            pItem->Base[0] = pItem->NextItem;

        pItem->PrevItem = NULL;
        pItem->NextItem = NULL;
        pItem->Base = NULL;
        pItem->Timeval = GetTimeval() + (timeout / Interval);

        if(InternalAddTimerItem(pItem) != 0)
        {
            delete pItem;
            m_TimerDataMap.erase(iter);
            return 0;
        }
    }

    void Clear(IdT timerId)
    {
        typename boost::unordered_map<IdT, TimerItem<DataT, IdT, TimeValueT>*>::iterator iter = m_TimerDataMap.find(timerId);
        if(iter == m_TimerDataMap.end())
        {
            return;
        }

        TimerItem<DataT, IdT, TimeValueT>* pItem = iter->second;
        if(pItem->NextItem)
            pItem->NextItem->PrevItem = pItem->PrevItem;

        if(pItem->PrevItem)
            pItem->PrevItem->NextItem = pItem->NextItem;
        else
            pItem->Base[0] = pItem->NextItem;

        delete pItem;

        m_TimerDataMap.erase(iter);
    }

    void CheckTimer()
    {
        TimeValueT now = GetTimeval();
        while(m_LastTimeval < now)
        {
            TimeValueT index = m_LastTimeval & TVR_MASK;

            if(!index && 
                !cascade(m_Vector2, ((m_LastTimeval >> TVR_BITS) & TVN_MASK)) &&
                !cascade(m_Vector3, ((m_LastTimeval >> (TVR_BITS + TVN_BITS)) & TVN_MASK)) &&
                !cascade(m_Vector4, ((m_LastTimeval >> (TVR_BITS + 2*TVN_BITS)) & TVN_MASK)))
            {
                cascade(m_Vector5, ((m_LastTimeval >> (TVR_BITS + 3*TVN_BITS)) & TVN_MASK));
            }

            ++m_LastTimeval;

            TimerItem<DataT, IdT, TimeValueT>* pTimerItem = m_Vector1[index];
            m_Vector1[index] = NULL;
            while(pTimerItem)
            {
                typename boost::unordered_map<IdT, TimerItem<DataT, IdT, TimeValueT>*>::iterator iter = m_TimerDataMap.find(pTimerItem->TimerId);
                if(iter != m_TimerDataMap.end())
                    m_TimerDataMap.erase(iter);

                try
                {
                    pTimerItem->Invoke();
                }
                catch(...)
                {
                    // nothing
                    LOG("error: timer list catch exception, you need check your code to catch the exception.");
                }

                TimerItem<DataT, IdT, TimeValueT>* pItem = pTimerItem;
                pTimerItem = pTimerItem->NextItem;

                delete pItem;
            }
        }
    }

    void Dump()
    {
        printf("Base Timeval: (0x%016lX)%lu\n", m_LastTimeval, m_LastTimeval);
        printf("////////////////////////////////////////////////////////////////////\n");

        printf("vector1:\n");
        PrintVector(m_Vector1, TVR_SIZE);
        printf("\n");

        printf("vector2:\n");
        PrintVector(m_Vector2, TVN_SIZE);
        printf("\n");

        printf("vector3:\n");
        PrintVector(m_Vector3, TVN_SIZE);
        printf("\n");

        printf("vector4:\n");
        PrintVector(m_Vector4, TVN_SIZE);
        printf("\n");

        printf("vector5:\n");
        PrintVector(m_Vector5, TVN_SIZE);
        printf("\n");

    }

};

template<typename DataT, int32_t Interval = TIMER_DEFAULT_INTERVAL>
class Timer :
    public TimerBase<DataT, uint32_t, uint64_t, Interval>
{
public:
    typedef uint32_t TimerID;
    typedef uint64_t TimeValue;

    TimerID SetTimeout(boost::function<void(DataT)> callback, int timeout, DataT data)
    {
        ++this->m_LastTimerId;

        TimerItem<DataT, TimerID, TimeValue>* pItem = new TimerItem<DataT, TimerID, TimeValue>();
        pItem->TimerId = this->m_LastTimerId;
        pItem->Timeval = this->GetTimeval() + (timeout / Interval);
        pItem->Data = data;
        pItem->Callback = callback;

        if(this->InternalAddTimerItem(pItem) != 0)
        {
            delete pItem;
            --this->m_LastTimerId;
            return 0;
        }

        this->m_TimerDataMap.insert(std::make_pair<TimerID, TimerItem<DataT, TimerID, TimeValue>*>(pItem->TimerId, pItem));
        return pItem->TimerId;
    }

    template<typename ServiceT>
    inline TimerID SetTimeout(ServiceT* pService, int timeout, DataT data)
    {
        return SetTimeout(boost::bind(&ServiceT::OnTimeout, pService, _1), timeout, data);
    }

    inline DataT* GetTimer(TimerID timerId)
    {
        typename boost::unordered_map<TimerID, TimerItem<DataT, TimerID, TimeValue>*>::iterator iter = this->m_TimerDataMap.find(timerId);
        if(iter == this->m_TimerDataMap.end())
            return NULL;
        else
            return &iter->second->Data;
    }
};
template<int32_t Interval>
class Timer<void, Interval> :
    public TimerBase<void, uint32_t, uint64_t, Interval>
{
public:
    typedef uint32_t TimerID;
    typedef uint64_t TimeValue;

    template<typename ServiceT>
    inline TimerID SetTimeout(ServiceT* pService, int timeout)
    {
        return SetTimeout(boost::bind(&ServiceT::OnTimeout, pService), timeout);
    }

    TimerID SetTimeout(boost::function<void()> callback, int timeout)
    {
        ++this->m_LastTimerId;

        TimerItem<void, TimerID, TimeValue>* pItem = new TimerItem<void, TimerID, TimeValue>();
        pItem->TimerId = this->m_LastTimerId;
        pItem->Timeval = this->GetTimeval() + (timeout / Interval);
        pItem->Callback = callback;

        if(this->InternalAddTimerItem(pItem) != 0)
        {
            delete pItem;
            --this->m_LastTimerId;
            return 0;
        }

        this->m_TimerDataMap.insert(std::make_pair<TimerID, TimerItem<void, TimerID, TimeValue>*>(pItem->TimerId, pItem));
        return pItem->TimerId;
    }
};

#endif // define __TIMER_HPP__
