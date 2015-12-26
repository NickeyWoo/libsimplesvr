/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __EVENTSCHEDULER_HPP__
#define __EVENTSCHEDULER_HPP__

#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>
#include <pthread.h>
#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <exception>
#include <time.h>
#include <sys/time.h>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "EPoll.hpp"
#include "Clock.hpp"
#include "Log.hpp"

template<typename PollT>
class EventSchedulerImpl :
    public boost::noncopyable
{
public:
    typedef PollT PollType;

    inline void RegisterIdleCallback(boost::function<void(void)> callback)
    {
        m_IdleCallbackList.push_back(callback);
    }

    inline void RegisterLoopCallback(boost::function<void(void)> callback)
    {
        m_LoopCallbackList.push_back(callback);
    }

    inline int GetIdleTimeout()
    {
        return m_IdleTimeout;
    }

    inline int SetIdleTimeout(int timeout)
    {
        int to = m_IdleTimeout;
        m_IdleTimeout = timeout;
        return to;
    }

    inline int CreateScheduler()
    {
        return m_Poll.CreatePoll();
    }

    inline void Close()
    {
        m_Poll.Close();
    }

    template<typename ServiceT>
    inline int UnRegister(ServiceT* pService)
    {
        return UnRegister(&pService->m_ServerInterface);
    }

    template<typename ChannelDataT>
    inline int UnRegister(ServerInterface<ChannelDataT>* pServerInterface)
    {
        return m_Poll.EventCtl(PollT::DEL, 0, pServerInterface->m_Channel.Socket, NULL);
    }

    template<typename ServiceT>
    inline int Update(ServiceT* pService, int events)
    {
        return Update(&pService->m_ServerInterface, events);
    }

    template<typename ChannelDataT>
    inline int Update(ServerInterface<ChannelDataT>* pServerInterface, int events)
    {
        int iRet = -1;
        if(-1 == (iRet = m_Poll.EventCtl(PollT::MOD, events, pServerInterface->m_Channel.Socket, pServerInterface)) && errno == ENOENT)
            return Register(pServerInterface, events);
        else
            return iRet;
    }

    template<typename ServiceT>
    inline int Register(ServiceT* pService, int events)
    {
        return Register(&pService->m_ServerInterface, events);
    }

    template<typename ChannelDataT>
    inline int Register(ServerInterface<ChannelDataT>* pServerInterface, int events)
    {
        return m_Poll.EventCtl(PollT::ADD, events, pServerInterface->m_Channel.Socket, pServerInterface);
    }

    template<typename ServiceT>
    inline int Wait(ServiceT* pService, int events, timeval* timeout = NULL)
    {
        return Wait(&pService->m_ServerInterface, events, timeout);
    }

    template<typename ChannelDataT>
    int Wait(ServerInterface<ChannelDataT>* pServerInterface, int events, timeval* tv = NULL)
    {
        pollfd stPollInfo;
        bzero(&stPollInfo, sizeof(pollfd));

        stPollInfo.fd = pServerInterface->m_Channel.Socket;
        stPollInfo.events = POLLIN;

        int timeout = -1;
        if(tv != NULL)
            timeout = (tv->tv_sec * 1000000 + tv->tv_usec) / 1000;

        return poll(&stPollInfo, 1, timeout);
    }

    void Dispatch()
    {
        LDEBUG_CLOCK_TRACE("start event dispatch loop...");
        while(!m_Quit)
        {
            ServerInterface<void>* pInterface = NULL;
            uint32_t events;
            int ready = m_Poll.WaitEvent(&pInterface, &events, m_IdleTimeout);
            try
            {
                if(ready > 0)
                {
                    if((events & PollT::ERR) == PollT::ERR)
                        pInterface->OnError();
                    else if((events & PollT::IN) == PollT::IN)
                        pInterface->OnReadable();
                    else if((events & PollT::OUT) == PollT::OUT)
                        pInterface->OnWriteable();
                }
                else if(ready == 0)
                {
                    for(std::list<boost::function<void(void)> >::iterator iter = m_IdleCallbackList.begin();
                        iter != m_IdleCallbackList.end();
                        ++iter)
                    {
                        (*iter)();
                    }
                }
                for(std::list<boost::function<void(void)> >::iterator iter = m_LoopCallbackList.begin();
                    iter != m_LoopCallbackList.end();
                    ++iter)
                {
                    (*iter)();
                }
            }
            catch(std::exception& error)
            {
                // ignore error
                LOG("unknown error: %s", error.what());
            }
        }
    }

    EventSchedulerImpl() :
        m_Quit(false),
        m_IdleTimeout(-1)
    {
    }

protected:
    bool m_Quit;
    int m_IdleTimeout;
    PollT m_Poll;
    std::list<boost::function<void(void)> > m_IdleCallbackList;
    std::list<boost::function<void(void)> > m_LoopCallbackList;
};

typedef EventSchedulerImpl<EPoll> EventScheduler;

#endif // define __EVENTSCHEDULER_HPP__
