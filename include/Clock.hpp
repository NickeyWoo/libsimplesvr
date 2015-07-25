/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __CLOCK_HPP__
#define __CLOCK_HPP__

#include <utility>
#include <list>
#include <string>
#include <time.h>
#include <boost/noncopyable.hpp>
#include "PoolObject.hpp"

#ifdef DEBUG
    #define DEBUG_CLOCK_TRACE(msg)                                                                                          \
        PoolObject<Clock>::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())
#else
    #define DEBUG_CLOCK_TRACE(msg)
#endif

#ifdef LDEBUG
    #define LDEBUG_CLOCK_TRACE(msg)                                                                                         \
        PoolObject<Clock>::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())
#else
    #define LDEBUG_CLOCK_TRACE(msg)
#endif

#define CLOCK_BEGIN(msg)                                                                                                    \
        PoolObject<Clock>::Instance().Clear();                                                                              \
        PoolObject<Clock>::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())

#define CLOCK_TIMESPAN()                                                                                                    \
        PoolObject<Clock>::Instance().Timespan()

#define CLOCK_DUMP()                                                                                                        \
        PoolObject<Clock>::Instance().Dump()

#define CLOCK_TRACE(msg)                                                                                                    \
        PoolObject<Clock>::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())

#define CLOCK_CLEAR()                                                                                                       \
        PoolObject<Clock>::Instance().Clear()

class Clock :
    public boost::noncopyable
{
public:
    uint64_t Tick();
    uint64_t Tick(const char* message);
    void Clear();
    double Timespan();

    std::string Dump();

private:
    std::list<std::pair<std::string, timeval> > m_ClockList;
};

#endif // define __CLOCK_HPP__
