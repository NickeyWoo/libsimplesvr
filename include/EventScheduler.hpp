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
#include <list>
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

	inline void RegisterIdleCallback(boost::function<void(void)> callback)
	{
		m_IdleCallbackList.push_back(callback);
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
					for(std::list<boost::function<void(void)> >::iterator iter = m_IdleCallbackList.begin();
						iter != m_IdleCallbackList.end();
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
};

typedef EventSchedulerImpl<EPoll> EventScheduler;

#endif // define __EVENTSCHEDULER_HPP__
