/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-19
 *
--*/
#ifndef __POOL_HPP__
#define __POOL_HPP__

#include <pthread.h>
#include <boost/noncopyable.hpp>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include "EPoll.hpp"
#include "Clock.hpp"
#include "EventScheduler.hpp"

template<typename SchedulerT>
class SimplePool :
	public boost::noncopyable
{
public:
	typedef SchedulerT SchedulerType;
	typedef typename SchedulerT::PollType PollType;

	static SimplePool& Instance()
	{
		static SimplePool instance;
		return instance;
	}

	template<typename T>
	inline int Register(T service, int events)
	{
		return m_Scheduler.Register(service, events);
	}

	int Startup()
	{
		m_Scheduler.Dispatch();
		return 0;
	}

	inline SchedulerType* GetScheduler()
	{
		return &m_Scheduler;
	}

	inline Clock* GetClock()
	{
		return &m_Clock;
	}

	inline uint32_t GetID()
	{
		return 0;
	}

protected:
	SchedulerType m_Scheduler;
	Clock m_Clock;
};

template<typename SchedulerT>
class ProcessPool :
	public boost::noncopyable
{
public:
	typedef SchedulerT SchedulerType;
	typedef typename SchedulerT::PollType PollType;

	static ProcessPool& Instance()
	{
		static ProcessPool instance;
		return instance;
	}

	template<typename ServiceT>
	inline int Register(ServiceT* pService, int events)
	{
		if(!m_bStartup)
		{
			m_vRegisterService.push_back(std::make_pair((ServerInterface<void>*)&pService->m_ServerInterface, events));
			return 0;
		}
		else
			return GetScheduler()->Register(pService, events);
	}

	int Startup(uint32_t num = 1)
	{
		for(uint32_t i = 1; i < num; ++i)
		{
			pid_t pid = fork();
			if(pid == -1)
				return -1;
			else if(pid == 0)
			{
				m_id = i;
				break;
			}
		}

		Clock clock;
		m_pClock = &clock;

		SchedulerType scheduler;
		m_pScheduler = &scheduler;

		for(std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterService.begin();
			iter != m_vRegisterService.end();
			++iter)
		{
			if(scheduler.Register(iter->first, iter->second) != 0)
				return -1;
		}

		m_bStartup = true;
		scheduler.Dispatch();
		return 0;
	}

	inline SchedulerType* GetScheduler()
	{
		return m_pScheduler;
	}

	inline Clock* GetClock()
	{
		return m_pClock;
	}

	inline uint32_t GetID()
	{
		return m_id;
	}

protected:
	ProcessPool() :
		m_bStartup(false),
		m_pScheduler(NULL),
		m_pClock(NULL),
		m_id(0)
	{
	}

	bool m_bStartup;
	SchedulerType* m_pScheduler;
	Clock* m_pClock;
	uint32_t m_id;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterService;
};

template<typename SchedulerT>
class ThreadPool :
	public boost::noncopyable
{
public:
	typedef SchedulerT SchedulerType;
	typedef typename SchedulerT::PollType PollType;

	static ThreadPool& Instance()
	{
		static ThreadPool instance;
		return instance;
	}

	template<typename ServiceT>
	inline int Register(ServiceT* pService, int events)
	{
		if(!m_bStartup)
		{
			m_vRegisterService.push_back(std::make_pair((ServerInterface<void>*)&pService->m_ServerInterface, events));
			return 0;
		}
		else
			return GetScheduler()->Register(pService, events);
	}

	int Startup(uint32_t num = 1)
	{
		m_bStartup = true;

		SchedulerType* pSchedulerBuffer = new SchedulerType[num];
		Clock* pClockBuffer = new Clock[num];
		for(uint32_t i = 1; i < num; ++i)
		{
			pthread_t tid;
			if(0 != pthread_create(&tid, NULL, ThreadPool<SchedulerT>::ThreadProc, &pSchedulerBuffer[i]))
				return -1;

			PoolData data;
			data.id = i;
			data.pScheduler = &pSchedulerBuffer[i];
			data.pClock = &pClockBuffer[i];
			m_PoolMap[tid] = data;

			for(std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterService.begin();
				iter != m_vRegisterService.end();
				++iter)
			{
				if(pSchedulerBuffer[i].Register(iter->first, iter->second) != 0)
					return -1;
			}
		}

		PoolData data;
		data.id = 0;
		data.pScheduler = &pSchedulerBuffer[0];
		data.pClock = &pClockBuffer[0];

		pthread_t tid = pthread_self();
		m_PoolMap[tid] = data;

		for(std::vector<std::pair<ServerInterface<void>*, int> >::iterator iter = m_vRegisterService.begin();
			iter != m_vRegisterService.end();
			++iter)
		{
			pSchedulerBuffer[0].Register(iter->first, iter->second);
		}

		ThreadPool<SchedulerT>::ThreadProc(&pSchedulerBuffer[0]);
		delete [] pSchedulerBuffer;
		delete [] pClockBuffer;
		return 0;
	}
	
	SchedulerType* GetScheduler()
	{
		pthread_t tid = pthread_self();
		typename std::map<pthread_t, PoolData>::iterator iter = m_PoolMap.find(tid);
		if(iter == m_PoolMap.end())
			return NULL;
		return iter->second.pScheduler;
	}

	inline Clock* GetClock()
	{
		pthread_t tid = pthread_self();
		typename std::map<pthread_t, PoolData>::iterator iter = m_PoolMap.find(tid);
		if(iter == m_PoolMap.end())
			return NULL;
		return iter->second.pClock;
	}

	inline uint32_t GetID()
	{
		pthread_t tid = pthread_self();
		typename std::map<pthread_t, PoolData>::iterator iter = m_PoolMap.find(tid);
		if(iter == m_PoolMap.end())
			return 0;
		return iter->second.id;
	}

	static void* ThreadProc(void* pArg)
	{
		SchedulerType* pScheduler = (SchedulerType*)pArg;
		pScheduler->Dispatch();
		return NULL;
	}

protected:
	ThreadPool() :
		m_bStartup(false)
	{
	}

	struct PoolData {
		uint32_t id;
		SchedulerType* pScheduler;
		Clock* pClock;
	};

	bool m_bStartup;
	std::map<pthread_t, PoolData> m_PoolMap;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterService;
};

#ifdef POOL_USE_PROCESSPOOL
	typedef ProcessPool<EventScheduler<EPoll> > Pool;
#else
	#ifdef POOL_USE_THREADPOOL
		typedef ThreadPool<EventScheduler<EPoll> > Pool;
	#else
		typedef SimplePool<EventScheduler<EPoll> > Pool;
	#endif
#endif

#endif // define __POOL_HPP__
