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
class MultiProcessEventScheduler :
	public boost::noncopyable
{
public:
	typedef PollT PollType;

	static MultiProcessEventScheduler<PollT>& Instance()
	{
		static MultiProcessEventScheduler<PollT> instance;
		return instance;
	}

	~MultiProcessEventScheduler()
	{
		m_Poll.Close();
	}

	template<typename ServerImplT>
	inline int UnRegister(ServerImplT* pServer)
	{
		return UnRegister(&pServer->m_ServerInterface);
	}

	template<typename ChannelDataT>
	inline int UnRegister(ServerInterface<ChannelDataT>* pServerInterface)
	{
		if(m_Startup)
			return m_Poll.EventCtl(PollT::DEL, 0, pServerInterface->m_Channel.fd, NULL);
		else
			return -1;
	}

	template<typename ServerImplT>
	inline int Update(ServerImplT* pServer, int events)
	{
		return Update(&pServer->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Update(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		if(m_Startup)
			return m_Poll.EventCtl(PollT::MOD, events, pServerInterface->m_Channel.fd, pServerInterface);
		else
			return -1;
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

	int Startup(uint32_t concurrent = 1)
	{
		if(m_Startup)
			return 0;

		for(uint32_t i = 1; i < concurrent; ++i)
		{
			pid_t pid = fork();
			if(pid == -1)
			{
				printf("error: create concurrent process fail, %s.\n", strerror(errno));
				return -1;
			}
			else if(pid == 0)
			{
				m_SchedulerId = i;
				break;
			}
		}

		if(m_Poll.CreatePoll() == -1)
		{
			printf("error: create poll fail, %s.\n", strerror(errno));
			return -1;
		}

		for(typename std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterInterface.begin();
			iter != m_vRegisterInterface.end();
			++iter)
		{
			if(m_Poll.EventCtl(PollT::ADD, iter->second, iter->first->m_Channel.fd, iter->first) == -1)
			{
				printf("error: set events fail, %s.\n", strerror(errno));
				return -1;
			}
		}

		m_Startup = true;
		Dispatch();
		return 0;
	}

	inline uint32_t GetSchedulerId()
	{
		return m_SchedulerId;
	}

protected:

	MultiProcessEventScheduler() :
		m_Startup(false),
		m_Quit(false),
		m_SchedulerId(0)
	{
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

	bool m_Startup;
	bool m_Quit;
	uint32_t m_SchedulerId;
	PollT m_Poll;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterInterface;
};

template<typename PollT>
class MultiThreadEventScheduler :
	public boost::noncopyable
{
public:
	typedef PollT PollType;

	static MultiThreadEventScheduler<PollT>& Instance()
	{
		static MultiThreadEventScheduler<PollT> instance;
		return instance;
	}

	~MultiThreadEventScheduler()
	{
		for(uint32_t i = 0; i < m_Concurrent; ++i)
			m_PollBuffer[i].Close();

		if(m_PollBuffer)
			delete [] m_PollBuffer;
	}

	template<typename ServerImplT>
	inline int UnRegister(ServerImplT* pServer)
	{
		return UnRegister(&pServer->m_ServerInterface);
	}

	template<typename ChannelDataT>
	int UnRegister(ServerInterface<ChannelDataT>* pServerInterface)
	{
		if(m_Startup)
		{
			pthread_t tid = pthread_self();
			typename std::map<pthread_t, std::pair<uint32_t, PollT*> >::iterator iter = m_ThreadPollMap.find(tid);
			if(iter == m_ThreadPollMap.end())
				return -1;
			return iter->second.second->EventCtl(PollT::DEL, 0, pServerInterface->m_Channel.fd, NULL);
		}
		else
			return -1;
	}

	template<typename ServerImplT>
	inline int Update(ServerImplT* pServer, int events)
	{
		return Update(&pServer->m_ServerInterface, events);
	}

	template<typename ChannelDataT>
	inline int Update(ServerInterface<ChannelDataT>* pServerInterface, int events)
	{
		if(m_Startup)
		{
			pthread_t tid = pthread_self();
			typename std::map<pthread_t, std::pair<uint32_t, PollT*> >::iterator iter = m_ThreadPollMap.find(tid);
			if(iter == m_ThreadPollMap.end())
				return -1;
			return iter->second.second->EventCtl(PollT::MOD, events, pServerInterface->m_Channel.fd, pServerInterface);
		}
		else
			return -1;
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
		{
			pthread_t tid = pthread_self();
			typename std::map<pthread_t, std::pair<uint32_t, PollT*> >::iterator iter = m_ThreadPollMap.find(tid);
			if(iter == m_ThreadPollMap.end())
				return -1;
			return iter->second.second->EventCtl(PollT::ADD, events, pServerInterface->m_Channel.fd, pServerInterface);
		}
		else
		{
			m_vRegisterInterface.push_back(std::make_pair((ServerInterface<void>*)pServerInterface, events));
			return 0;
		}
	}

	int Startup(uint32_t concurrent = 1)
	{
		if(m_Startup)
			return 0;

		m_Concurrent = concurrent;
		m_PollBuffer = new PollT[m_Concurrent];

		for(uint32_t i = 1; i < m_Concurrent; ++i)
		{
			if(m_PollBuffer[i].CreatePoll() == -1)
			{
				printf("error: create poll fail, %s.\n", strerror(errno));
				return -1;
			}

			for(typename std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterInterface.begin();
				iter != m_vRegisterInterface.end();
				++iter)
			{
				if(m_PollBuffer[i].EventCtl(PollT::ADD, iter->second, iter->first->m_Channel.fd, iter->first) == -1)
				{
					printf("error: set events fail, %s.\n", strerror(errno));
					return -1;
				}
			}

			pthread_t tid;
			if(0 != pthread_create(&tid, NULL, MultiThreadEventScheduler<PollT>::ThreadProc, &m_PollBuffer[i]))
			{
				printf("error: create concurrent pthread fail, %s.\n", strerror(errno));
				return -1;
			}
			m_ThreadPollMap[tid] = std::make_pair(i, &m_PollBuffer[i]);
		}

		if(m_PollBuffer[0].CreatePoll() == -1)
		{
			printf("error: create poll fail, %s.\n", strerror(errno));
			return -1;
		}

		for(typename std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterInterface.begin();
			iter != m_vRegisterInterface.end();
			++iter)
		{
			if(m_PollBuffer[0].EventCtl(PollT::ADD, iter->second, iter->first->m_Channel.fd, iter->first) == -1)
			{
				printf("error: set events fail, %s.\n", strerror(errno));
				return -1;
			}
		}

		pthread_t tid = pthread_self();
		m_ThreadPollMap[tid] = std::make_pair(0, &m_PollBuffer[0]);

		m_Startup = true;
		MultiThreadEventScheduler<PollT>::ThreadProc(m_PollBuffer);
		return 0;
	}

	uint32_t GetSchedulerId()
	{
		pthread_t tid = pthread_self();
		typename std::map<pthread_t, std::pair<uint32_t, PollT*> >::iterator iter = m_ThreadPollMap.find(tid);
		return iter->second.first;
	}

protected:

	static void* ThreadProc(void* pData)
	{
		PollT* poll = (PollT*)pData;
		while(true)
		{
			ServerInterface<void>* pInterface = NULL;
			uint32_t events;
			if(poll->WaitEvent(&pInterface, &events, -1) > 0)
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
		return NULL;
	}

	MultiThreadEventScheduler() :
		m_Startup(false),
		m_Concurrent(0),
		m_PollBuffer(NULL)
	{
	}

	bool m_Startup;
	uint32_t m_Concurrent;
	PollT* m_PollBuffer;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterInterface;
	std::map<pthread_t, std::pair<uint32_t, PollT*> > m_ThreadPollMap;
};

#ifdef EVENTSCHEDULER_USE_THREAD
typedef MultiThreadEventScheduler<EPoll> EventScheduler;
#else
typedef MultiProcessEventScheduler<EPoll> EventScheduler;
#endif

#endif // define __EVENTSCHEDULER_HPP__
