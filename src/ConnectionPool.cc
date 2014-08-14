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
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <utility>
#include <string>
#include <list>
#include <vector>
#include <map>
#include "Pool.hpp"
#include "PoolObject.hpp"
#include "Timer.hpp"
#include "ConnectionPool.hpp"

ConnectionPool::ConnectionPool() :
	m_MaxConnection(CONNECTIONPOOL_MAXCONNECTION),
	m_IdleConnectionTimeout(CONNECTIONPOOL_IDLE_TIMEOUT)
{
}

int ConnectionPool::Attach(sockaddr_in* pAddr)
{
	return 0;
}

void ConnectionPool::Detach(int sockFd)
{
	// PoolObject<Timer<int, TIMER_DEFAULT_INTERVAL>::Instance().SetTimeout(this, m_IdleConnectionTimeout, ); 
}



