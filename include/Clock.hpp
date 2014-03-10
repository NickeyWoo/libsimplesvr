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

class Clock
{
public:
	static uint64_t Tick();
	static uint64_t Tick(const char* message);
	static void Clear();

	static void Dump();

private:
	static std::list<std::pair<std::string, timespec> > m_ClockList;
};

#endif // define __CLOCK_HPP__
