/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-19
 *
--*/
#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <sys/time.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include "Pool.hpp"
#include "hashtable.hpp"

template<typename DataT>
class Timer :
	public boost::noncopyable
{
public:
	template<typename ServiceT>
	int SetTimeout(ServiceT* pService, int timeout, DataT data)
	{
		if(!m_bInit)
			return -1;

		timeval tv;
		if(-1 == gettimeofday(&tv, NULL))
			return -1;

		int millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;
		boost::function<void(DataT)> callback = boost::bind(&ServiceT::OnTimeout, pService, _1);
		return 0;
	}

	DataT GetTimer(int timerId)
	{
		if(!m_bInit)
			return;

	}

	void Update(int timerId, int timeout)
	{
		if(!m_bInit)
			return;

	}

	void Clear(int timerId)
	{
		if(!m_bInit)
			return;

	}

	void CheckTimer()
	{

	}

	static bool OnStartup()
	{
		Timer<DataT>& timer = Timer<DataT>::Instance();
		return timer.Initialize();
	}

	Timer() :
		m_bInit(false)
	{
		if(!Pool::Instance().IsStartup())
			Pool::Instance().RegisterStartupCallback(boost::bind(&Timer<DataT>::OnStartup), true);
		else
			Initialize();
	}

protected:
	bool Initialize()
	{
		EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
		if(scheduler.GetIdleTimeout() > CheckInterval)
			scheduler.SetIdleTimeout(CheckInterval);
		scheduler.RegisterIdleCallback(boost::bind(&Timer<DataT>::CheckTimer, &timer));

		m_ht = TimerHashTable<uint32_t, DataT>::CreateHashTable(, 30);
		m_ht.SetDefaultTimeout(CheckInterval / 1000 + 1);

		timer.m_bInit = true;
		return true;

	}

	TimerHashTable<uint32_t, DataT> m_ht;















	bool m_bInit;
};

#endif // define __TIMER_HPP__
