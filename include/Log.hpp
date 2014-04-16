/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-20
 *
--*/
#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <utility>
#include <vector>
#include <string>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>

extern const char* safe_strerror(int error);

class TextFormat
{
public:
	TextFormat()
	{
	}

	TextFormat(const char* str) :
		m_str(str)
	{
	}

	std::string m_str;
};

template<typename FormatT = TextFormat>
class Log :
	public boost::noncopyable
{
public:
	Log<FormatT>& operator << (TextFormat tf)
	{
		printf("Log: %s\n", tf.m_str.c_str());
		return *this;
	}

protected:
	FormatT m_Format;
};


#endif // define __LOG_HPP__
