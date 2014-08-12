/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-08-12
 *
--*/
#ifndef __CONNECTIONPOOL_HPP__
#define __CONNECTIONPOOL_HPP__

#include <pthread.h>
#include <utility>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "PoolObject.hpp"
#include "Clock.hpp"
#include "Log.hpp"

template<typename T>
class ConnectionPool :
	public boost::noncopyable
{
};


#endif // define __CONNECTIONPOOL_HPP__
