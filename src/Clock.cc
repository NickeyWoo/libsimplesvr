/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
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
#include <string>
#include <boost/format.hpp>
#include "Clock.hpp"

uint64_t Clock::Tick()
{
    timeval tv;
    if(gettimeofday(&tv, NULL) == -1)
        return 0;
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

uint64_t Clock::Tick(const char* message)
{
    timeval tv;
    if(gettimeofday(&tv, NULL) == -1)
        return 0;
    
    if(message)
        m_ClockList.push_back(std::make_pair(std::string(message), tv));
    else
        m_ClockList.push_back(std::make_pair(std::string(), tv));

    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void Clock::Clear()
{
    m_ClockList.clear();
}

double Clock::Timespan()
{
    if(m_ClockList.empty())
        return 0;

    timeval tvs = m_ClockList.front().second;
    timeval tve = m_ClockList.back().second;

    return ((double)(tve.tv_sec*1000000+tve.tv_usec) - (tvs.tv_sec*1000000+tvs.tv_usec)) / 1000;
}

std::string Clock::Dump()
{
    std::list<std::pair<std::string, timeval> >::iterator iter = m_ClockList.begin();
    if(iter == m_ClockList.end())
        return std::string();

    std::string strLogInfo;
    uint64_t last = iter->second.tv_sec * 1000000 + iter->second.tv_usec;
    for(; iter != m_ClockList.end(); ++iter)
    {
        uint64_t cur = iter->second.tv_sec * 1000000 + iter->second.tv_usec;
        strLogInfo.append((boost::format("[+%.02fms] %s\n") % ((double)(cur - last) / 1000) % iter->first).str());
        last = cur;
    }
    return strLogInfo;
}


