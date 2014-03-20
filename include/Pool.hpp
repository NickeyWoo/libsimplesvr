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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utility>
#include <string>
#include <vector>
#include <map>
#include "EventScheduler.hpp"

class ProcessPool :
	public boost::noncopyable
{
public:
	static ProcessPool& Instance();
	int Startup(uint32_t num = 1);

	inline void RegisterStartupCallback(boost::function<bool(void)>& callback)
	{
		m_StartupCallbackVector.push_back(callback);
	}

	inline void RegisterIdleCallback(boost::function<void(void)>& callback)
	{
		m_IdleCallbackVector.push_back(callback);
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
		{
			EventScheduler& scheduler = EventScheduler::Instance();
			return scheduler.Register(pService, events);
		}
	}

	inline uint32_t GetID()
	{
		return m_id;
	}

protected:
	ProcessPool();

	bool m_bStartup;
	uint32_t m_id;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterService;
	std::vector<boost::function<bool(void)> > m_StartupCallbackVector;
	std::vector<boost::function<void(void)> > m_IdleCallbackVector;
};

class ThreadPool :
	public boost::noncopyable
{
public:
	static ThreadPool& Instance();
	int Startup(uint32_t num = 1);

	inline void RegisterStartupCallback(boost::function<bool(void)>& callback)
	{
		m_StartupCallbackVector.push_back(callback);
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
		{
			EventScheduler& scheduler = EventScheduler::Instance();
			return scheduler.Register(pService, events);
		}
	}

	uint32_t GetID();

protected:
	ThreadPool();
	static void* ThreadProc(void* paramenter);

	bool m_bStartup;
	std::vector<std::pair<ServerInterface<void>*, int> > m_vRegisterService;
	std::vector<boost::function<bool(void)> > m_StartupCallbackVector;
};

#if defined(POOL_USE_THREADPOOL)
	typedef ThreadPool Pool;
#else
	typedef ProcessPool Pool;
#endif

#endif // define __POOL_HPP__
