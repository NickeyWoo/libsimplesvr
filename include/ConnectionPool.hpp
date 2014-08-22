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
#include "Channel.hpp"
#include "Server.hpp"
#include "TcpClient.hpp"
#include "Timer.hpp"
#include "Clock.hpp"
#include "Log.hpp"

#define CONNECT_IDLE    0
#define CONNECT_BUSY    1

#define CONNECTIONPOOL_MAXCONNECTION        50
#define CONNECTIONPOOL_IDLE_TIMEOUT         30000       // 30s timeout

template<typename TcpClientT, int TimerInterval>
struct ConnectionInfo
{
    TcpClientT stClient;
    uint32_t dwFlags;
    typename Timer<TcpClientT*, TimerInterval>::TimerID IdleConnTimerId;
};

template<typename TcpClientT, 
            size_t MaxConn = CONNECTIONPOOL_MAXCONNECTION, size_t IdleConnTimeout = CONNECTIONPOOL_IDLE_TIMEOUT,
            int TimerInterval = TIMER_DEFAULT_INTERVAL>
class ConnectionPool :
    public boost::noncopyable
{
public:
    ConnectionPool() :
        m_MaxConnection(MaxConn),
        m_IdleConnectionTimeout(IdleConnTimeout)
    {
    }

    int Attach(TcpClientT** ppstClient)
    {
        ConnectionInfo<TcpClientT, TimerInterval> info;

        PoolObject<Timer<TcpClientT*, TimerInterval> >::Instance().Clear(info.IdleConnTimerId);
        *ppstClient = &info.stClient;
        return 0;
    }

    void Detach(TcpClientT* pstClient)
    {
    }

    void OnTimeout(TcpClientT* pstClient)
    {
        pstClient->Disconnect();
    }

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
