/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
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
#include <list>
#include <map>
#include "PoolObject.hpp"
#include "EventScheduler.hpp"

class ProcessPool :
	public boost::noncopyable
{
public:
	static ProcessPool& Instance();
	int Startup(uint32_t num = 1);

	inline bool IsStartup()
	{
		return m_bStartup;
	}

	inline void RegisterStartupCallback(boost::function<bool(void)> callback, bool front = false)
	{
		if(front)
			m_StartupCallbackList.push_front(callback);
		else
			m_StartupCallbackList.push_back(callback);
	}

	inline uint32_t GetID()
	{
		return m_id;
	}

	inline int SetIdleTimeout(int timeout)
	{
		if(!m_bStartup)
		{
			int old = m_IdleTimeout;
			m_IdleTimeout = timeout;
			return old;
		}
		else
			return PoolObject<EventScheduler>::Instance().SetIdleTimeout(timeout);
	}

	inline int GetIdleTimeout()
	{
		if(!m_bStartup)
			return m_IdleTimeout;
		else
			return PoolObject<EventScheduler>::Instance().GetIdleTimeout();
	}

protected:
	ProcessPool();

	bool m_bStartup;
	uint32_t m_id;
	int m_IdleTimeout;
	std::list<boost::function<bool(void)> > m_StartupCallbackList;
};

class ThreadPool :
	public boost::noncopyable
{
public:
	static ThreadPool& Instance();
	int Startup(uint32_t num = 1);

	inline bool IsStartup()
	{
		return m_bStartup;
	}

	inline void RegisterStartupCallback(boost::function<bool(void)> callback, bool front = false)
	{
		if(front)
			m_StartupCallbackList.push_front(callback);
		else
			m_StartupCallbackList.push_back(callback);
	}

	inline int SetIdleTimeout(int timeout)
	{
		if(!m_bStartup)
		{
			int old = m_IdleTimeout;
			m_IdleTimeout = timeout;
			return old;
		}
		else
			return PoolObject<EventScheduler>::Instance().SetIdleTimeout(timeout);
	}

	inline int GetIdleTimeout()
	{
		if(!m_bStartup)
			return m_IdleTimeout;
		else
			return PoolObject<EventScheduler>::Instance().GetIdleTimeout();
	}

	uint32_t GetID();

protected:
	ThreadPool();
	static void* ThreadProc(void* paramenter);

	bool m_bStartup;
	int m_IdleTimeout;
	std::list<boost::function<bool(void)> > m_StartupCallbackList;
};

#if defined(POOL_USE_THREADPOOL)
	typedef ThreadPool Pool;
#else
	typedef ProcessPool Pool;
#endif

#endif // define __POOL_HPP__
