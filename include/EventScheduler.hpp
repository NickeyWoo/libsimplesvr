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
#include <vector>
#include <map>
#include <exception>
#include <boost/noncopyable.hpp>
#include "Configure.hpp"
#include "EPoll.hpp"
#include "Server.hpp"

template<typename PollT>
class EventSchedulerImpl :
	public boost::noncopyable
{
public:
	typedef PollT PollType;

	static EventSchedulerImpl<PollT>& Instance()
	{
		static EventSchedulerImpl<PollT> instance;
		return instance;
	}

	template<typename ServerImplT>
	inline int UnRegister(ServerImplT* pServer)
	{
		return UnRegister(&pServer->m_ServerInterface);
	}

	template<typename ChannelDataT>
	inline int UnRegister(ServerInterface<ChannelDataT>* pServerInterface)
	{
		return m_Poll.EventCtl(PollT::DEL, 0, pServerInterface->m_Channel.fd, NULL);
	}

	template<typename ServerImplT>
	inline int Update(ServerImplT* pServer, int events)
	{
		return Update(&pServer->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Update(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		return m_Poll.EventCtl(PollT::MOD, events, pServerInterface->m_Channel.fd, pServerInterface);
	}

	template<typename ServerImplT>
	inline int Register(ServerImplT* pServer, int events)
	{
		return Register(&pServer->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Register(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		if(m_Startup)
			return m_Poll.EventCtl(PollT::ADD, events, pServerInterface->m_Channel.fd, pServerInterface);
		else
		{
			m_vRegisterInterface.push_back(std::make_pair((ServerInterface<void>*)pServerInterface, events));
			return 0;
		}
	}

	int Startup()
	{
		std::map<std::string, std::string> stConcurrentConfig = Configure::Get("global");
		uint32_t concurrent = 1;
		if(!stConcurrentConfig["concurrent"].empty())
			concurrent = strtoul(stConcurrentConfig["concurrent"].c_str(), NULL, 10);
		for(uint32_t i = 0; i < concurrent; ++i)
		{
			pid_t pid = fork();
			if(pid == -1)
			{
				printf("error: create concurrent process fail, %s.\n", strerror(errno));
				return -1;
			}
			else if(pid == 0)
				break;
		}

		if(m_Poll.Create() == -1)
			return -1;

		for(typename std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterInterface.begin();
			iter != m_vRegisterInterface.end();
			++iter)
		{
			if(m_Poll.EventCtl(PollT::ADD, iter->second, iter->first->m_Channel.fd, iter->first) == -1)
				return -1;
		}

		m_Startup = true;
		Dispatch();
		return 0;
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

protected:

	EventSchedulerImpl() :
		m_Startup(false),
		m_Quit(false)
	{
	}

	bool m_Startup;
	bool m_Quit;
	PollT m_Poll;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterInterface;
};

typedef EventSchedulerImpl<EPoll> EventScheduler;


#endif // define __EVENTSCHEDULER_HPP__
