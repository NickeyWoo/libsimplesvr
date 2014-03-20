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

template<typename DataT, int CheckInterval>
class Timer :
	public boost::noncopyable
{
public:

#if defined(POOL_USE_THREADPOOL)

	static pthread_once_t timer_once;
	static pthread_key_t timer_key;

	static void timer_free(void* buffer)
	{
		Timer<DataT, CheckInterval>* pTimer = (Timer<DataT, CheckInterval>*)buffer;
		delete pTimer;
	}

	static void timer_key_init()
	{
		pthread_key_create(&Timer<DataT, CheckInterval>::timer_key, &Timer<DataT, CheckInterval>::timer_free);
	}

	static Timer<DataT, CheckInterval>& Instance()
	{
		pthread_once(&Timer<DataT, CheckInterval>::timer_once, &Timer<DataT, CheckInterval>::timer_key_init);
		Timer<DataT, CheckInterval>* pTimer = (Timer<DataT, CheckInterval>*)pthread_getspecific(Timer<DataT, CheckInterval>::timer_key);
		if(!pTimer)
		{
			pTimer = new Timer<DataT, CheckInterval>();
			pthread_setspecific(Timer<DataT, CheckInterval>::timer_key, pTimer);
		}
		return *pTimer;
	}

#else
	static Timer<DataT, CheckInterval>& Instance()
	{
		static Timer<DataT, CheckInterval> timer;
		return timer;
	}
#endif

	template<typename ServiceT>
	int SetTimeout(ServiceT* pService, int timeout, DataT data)
	{
		if(!Pool::Instance().IsStartup())
			return -1;

#if !defined(TIMER_NONEED_CHECKINTERVAL)
		EventScheduler& scheduler = EventScheduler::Instance();
		if(scheduler.GetIdleTimeout() > CheckInterval)
			scheduler.SetIdleTimeout(CheckInterval);
#endif

		timeval tv;
		if(-1 == gettimeofday(&tv, NULL))
			return -1;

		int millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;
		boost::function<void(DataT)> callback = boost::bind(&ServiceT::OnTimeout, pService, _1);
		return 0;
	}

	DataT GetTimer(int timerId)
	{
		if(!Pool::Instance().IsStartup())
			return -1;

	}

	void Update(int timerId, int timeout)
	{
		if(!Pool::Instance().IsStartup())
			return -1;

	}

	void Clear(int timerId)
	{
		if(!Pool::Instance().IsStartup())
			return -1;

	}

	void CheckTimer()
	{

	}

protected:

};

#if defined(POOL_USE_THREADPOOL)

template<typename DataT, int CheckInterval>
pthread_once_t Timer<DataT, CheckInterval>::timer_once = PTHREAD_ONCE_INIT;

template<typename DataT, int CheckInterval>
pthread_key_t Timer<DataT, CheckInterval>::timer_key;

#endif

#endif // define __TIMER_HPP__
