/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __EVENTSCHEDULER_HPP__
#define __EVENTSCHEDULER_HPP__

#include <utility>
#include <string>
#include <exception>
#include <boost/noncopyable.hpp>
#include "EPoll.hpp"
#include "Server.hpp"

template<typename PollT>
class EventSchedulerImpl :
	public boost::noncopyable
{
public:
	typedef PollT PollType;

	EventSchedulerImpl() :
		m_Quit(false)
	{
	}

	template<typename DataT>
	int UnRegister(ServerInterface<DataT>* pInterface)
	{
		return m_Poll.EventCtl(PollT::DEL, 0, pInterface->m_Channel.fd, NULL);
	}

	template<typename DataT>
	int Register(ServerInterface<DataT>* pInterface, int events)
	{
		return m_Poll.EventCtl(PollT::ADD, events, pInterface->m_Channel.fd, pInterface);
	}

	void Dispatch()
	{
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

private:
	bool m_Quit;
	PollT m_Poll;
};

typedef EventSchedulerImpl<EPoll> EventScheduler;

#endif // define __EVENTSCHEDULER_HPP__
