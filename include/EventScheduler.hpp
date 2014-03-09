/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __EVENTSCHEDULER_HPP__
#define __EVENTSCHEDULER_HPP__

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
	void UnRegister(ServerInterface<DataT>* pInterface)
	{
		m_Poll.EventCtl(PollT::DEL, 0, pInterface->m_Channel.fd, NULL);
	}

	template<typename DataT>
	void Register(ServerInterface<DataT>* pInterface, int events)
	{
		m_Poll.EventCtl(PollT::ADD, events, pInterface->m_Channel.fd, pInterface);
	}

	void Dispatch()
	{
		while(!m_Quit)
		{
			ServerInterface<void>* pInterface = NULL;
			uint32_t events;
			if(m_Poll.WaitEvent(&pInterface, &events, -1) > 0)
			{
				if((events & PollT::POLLIN) == PollT::POLLIN)
					pInterface->OnReadable();
				else if((events & PollT::POLLOUT) == PollT::POLLOUT)
					pInterface->OnWriteable();
			}
		}
	}

private:
	bool m_Quit;
	PollT m_Poll;
};

typedef EventSchedulerImpl<EPoll> EventScheduler;

#endif // define __EVENTSCHEDULER_HPP__
