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
#include "Timer.hpp"
#include "Clock.hpp"
#include "Log.hpp"

#define CONNECT_IDLE	0
#define CONNECT_BUSY	1

#define CONNECTIONPOOL_MAXCONNECTION		50
#define CONNECTIONPOOL_IDLE_TIMEOUT			30000		// 30s timeout

struct ConnectFd {
	int sockfd;
	uint32_t dwFlags;
};

struct Connection {
	sockaddr_in Address;
	uint32_t Status;
};

class ConnectionPool :
	public boost::noncopyable
{
public:
	ConnectionPool();
	int Attach(sockaddr_in* pAddr);
	void Detach(int fd);
	void OnTimeout();

	inline void SetMaxConnection(uint32_t max)
	{
		m_MaxConnection = max;
	}

	inline void SetIdleTimeout(uint32_t timeout)
	{
		m_IdleConnectionTimeout = timeout;
	}

private:
	uint32_t m_MaxConnection;
	uint32_t m_IdleConnectionTimeout;
};


#endif // define __CONNECTIONPOOL_HPP__
