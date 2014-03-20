/*++
 *
 * Simple Server Library
 * Author: NickWu
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

#ifdef DEBUG
	#define DEBUG_CLOCK_TRACE(msg)		\
		Clock::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())
#else
	#define DEBUG_CLOCK_TRACE(msg)
#endif

#ifdef LDEBUG
	#define LDEBUG_CLOCK_TRACE(msg)	\
		Clock::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())
#else
	#define LDEBUG_CLOCK_TRACE(msg)
#endif

#define CLOCK_TRACE(msg)	\
		Clock::Instance().Tick((boost::format("[%s:%d] %s") % __FUNCTION__ % __LINE__ % msg).str().c_str())
#define CLOCK_CLEAR()	\
		Clock::Instance().Clear()

class Clock :
	public boost::noncopyable
{
public:
	static Clock& Instance();
	uint64_t Tick();
	uint64_t Tick(const char* message);
	void Clear();

	void Dump();

private:
	Clock();
	std::list<std::pair<std::string, timespec> > m_ClockList;
};

#endif // define __CLOCK_HPP__
