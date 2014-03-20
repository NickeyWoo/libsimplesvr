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
	template<typename ServiceT>
	int SetTimeout(ServiceT* pService, int timeout, DataT data)
	{
		timeval tv;
		if(-1 == gettimeofday(&tv, NULL))
			return -1;

		int millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;
		boost::function<void(DataT)> callback = boost::bind(&ServiceT::OnTimeout, pService, _1);
		return 0;
	}

	DataT* GetTimer(int timerId)
	{
	}

	void Update(int timerId, int timeout)
	{
	}

	void Clear(int timerId)
	{
	}

	void CheckTimer()
	{
	}

protected:

};

template<int CheckInterval>
class Timer<void, CheckInterval> :
	public boost::noncopyable
{
public:
	template<typename ServiceT>
	int SetTimeout(ServiceT* pService, int timeout)
	{
		timeval tv;
		if(-1 == gettimeofday(&tv, NULL))
			return -1;

		int millisec = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;
		boost::function<void()> callback = boost::bind(&ServiceT::OnTimeout, pService);
		return 0;
	}

	void Update(int timerId, int timeout)
	{
	}

	void Clear(int timerId)
	{
	}

	void CheckTimer()
	{
	}

protected:

};

#endif // define __TIMER_HPP__
