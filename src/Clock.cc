#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include "Clock.hpp"

#if defined(POOL_USE_THREADPOOL)

pthread_once_t clock_once = PTHREAD_ONCE_INIT;
pthread_key_t clock_key;

void clock_free(void* buffer)
{
	Clock* pClock = (Clock*)buffer;
	delete pClock;
}

void clock_key_init()
{
	pthread_key_create(&clock_key, &clock_free);
}

Clock& Clock::Instance()
{
	pthread_once(&clock_once, &clock_key_init);
	Clock* pClock = (Clock*)pthread_getspecific(clock_key);
	if(!pClock)
	{
		pClock = new Clock();
		pthread_setspecific(clock_key, pClock);
	}
	return *pClock;
}

#else

Clock& Clock::Instance()
{
	static Clock instance;
	return instance;
}

#endif

Clock::Clock()
{
}

uint64_t Clock::Tick()
{
	timespec ts;
	if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
		return 0;
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

uint64_t Clock::Tick(const char* message)
{
	timespec ts;
	if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
		return 0;
	
	if(message)
		m_ClockList.push_back(std::make_pair(std::string(message), ts));
	else
		m_ClockList.push_back(std::make_pair(std::string(), ts));

	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

void Clock::Clear()
{
	m_ClockList.clear();
}

void Clock::Dump()
{
	std::list<std::pair<std::string, timespec> >::iterator iter = m_ClockList.begin();
	if(iter == m_ClockList.end())
		return;

	uint64_t last = iter->second.tv_sec * 1000000000 + iter->second.tv_nsec;
	for(; iter != m_ClockList.end(); ++iter)
	{
		uint64_t cur = iter->second.tv_sec * 1000000000 + iter->second.tv_nsec;
		printf("[+%.02fms] %s\n", (double)(cur - last) / 1000000, iter->first.c_str());
		last = cur;
	}
}


