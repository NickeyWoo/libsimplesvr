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
#include "EPoll.hpp"
#include "Server.hpp"

template<typename PollT>
class EventScheduler:
	public boost::noncopyable
{
public:
	typedef PollT PollType;

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
			if(m_Poll.WaitEvent(&pInterface, &events, -1) > 0)
			{
				try
				{
					if((events & PollT::POLLIN) == PollT::POLLIN)
						pInterface->OnReadable();
					else if((events & PollT::POLLOUT) == PollT::POLLOUT)
						pInterface->OnWriteable();
				}
				catch(std::exception& error)
				{
					printf("%s\n", error.what());
				}
			}
		}
	}

	EventScheduler() :
		m_Quit(false)
	{
	}

protected:
	bool m_Quit;
	PollT m_Poll;
};

#endif // define __EVENTSCHEDULER_HPP__
