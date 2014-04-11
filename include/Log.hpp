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

template<typename BinT>
class Log :
	public boost::noncopyable
{
public:

};


#endif // define __LOG_HPP__
