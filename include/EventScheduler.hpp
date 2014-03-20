/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __EVENTSCHEDULER_HPP__
#define __EVENTSCHEDULER_HPP__

#include <pthread.h>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "EPoll.hpp"
#include "Clock.hpp"

template<typename PollT>
class EventSchedulerImpl :
	public boost::noncopyable
{
public:
	typedef PollT PollType;

#if defined(POOL_USE_THREADPOOL)

	static pthread_once_t scheduler_once;
	static pthread_key_t scheduler_key;

	static void scheduler_free(void* buffer)
	{
		EventSchedulerImpl<PollT>* pScheduler = (EventSchedulerImpl<PollT>*)buffer;
		delete pScheduler;
	}

	static void scheduler_key_init()
	{
		pthread_key_create(&EventSchedulerImpl<PollT>::scheduler_key, &EventSchedulerImpl<PollT>::scheduler_free);
	}

	static EventSchedulerImpl<PollT>& Instance()
	{
		pthread_once(&EventSchedulerImpl<PollT>::scheduler_once, &EventSchedulerImpl<PollT>::scheduler_key_init);
		EventSchedulerImpl<PollT>* pScheduler = (EventSchedulerImpl<PollT>*)pthread_getspecific(EventSchedulerImpl<PollT>::scheduler_key);
		if(!pScheduler)
		{
			pScheduler = new EventSchedulerImpl<PollT>();
			pthread_setspecific(EventSchedulerImpl<PollT>::scheduler_key, pScheduler);
		}
		return *pScheduler;
	}

#else
	static EventSchedulerImpl<PollT>& Instance()
	{
		static EventSchedulerImpl<PollT> instance;
		return instance;
	}
#endif

	inline void RegisterIdleCallback(boost::function<void(void)>& callback)
	{
		m_IdleCallbackVector.push_back(callback);
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
		return m_Poll.EventCtl(PollT::DEL, 0, pServerInterface->m_Channel.fd, NULL);
	}

	template<typename ServiceT>
	inline int Update(ServiceT* pService, int events)
	{
		return Update(&pService->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Update(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		return m_Poll.EventCtl(PollT::MOD, events, pServerInterface->m_Channel.fd, pServerInterface);
	}

	template<typename ServiceT>
	inline int Register(ServiceT* pService, int events)
	{
		return Register(&pService->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Register(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		return m_Poll.EventCtl(PollT::ADD, events, pServerInterface->m_Channel.fd, pServerInterface);
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
					if((events & PollT::POLLIN) == PollT::POLLIN)
						pInterface->OnReadable();
					else if((events & PollT::POLLOUT) == PollT::POLLOUT)
						pInterface->OnWriteable();
				}
				else if(ready == 0)
				{
					for(std::vector<boost::function<void(void)> >::iterator iter = m_IdleCallbackVector.begin();
						iter != m_IdleCallbackVector.end();
						++iter)
					{
						(*iter)();
					}
				}
			}
			catch(std::exception& error)
			{
				printf("%s\n", error.what());
			}
		}
	}

protected:
	EventSchedulerImpl() :
		m_Quit(false),
		m_IdleTimeout(-1)
	{
	}

	bool m_Quit;
	int m_IdleTimeout;
	PollT m_Poll;
	std::vector<boost::function<void(void)> > m_IdleCallbackVector;
};

#if defined(POOL_USE_THREADPOOL)

template<typename PollT>
pthread_once_t EventSchedulerImpl<PollT>::scheduler_once = PTHREAD_ONCE_INIT;

template<typename PollT>
pthread_key_t EventSchedulerImpl<PollT>::scheduler_key;

#endif

typedef EventSchedulerImpl<EPoll> EventScheduler;

#endif // define __EVENTSCHEDULER_HPP__
