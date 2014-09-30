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
#include "LoadBalance.hpp"
#include "Server.hpp"
#include "TcpClient.hpp"
#include "Timer.hpp"
#include "Clock.hpp"
#include "Log.hpp"

#define CONNECTIONPOOL_MAXCONNECTION        100
#define CONNECTIONPOOL_IDLE_TIMEOUT         300000      // 5 min timeout
#define CONNECTIONPOOL_KEEPCONNECTION       ((size_t)0)

template<typename TcpClientT, int TimerInterval>
struct ConnectionInfo
{
    TcpClientT stClient;
    sockaddr_in stAddress;
    typename Timer<ConnectionInfo<TcpClientT, TimerInterval>*, 
        TimerInterval>::TimerID IdleConnTimerId;
};

template<typename TcpClientT, 
            size_t IdleConnTimeout = CONNECTIONPOOL_IDLE_TIMEOUT, size_t MaxConn = CONNECTIONPOOL_MAXCONNECTION,
            int TimerInterval = TIMER_DEFAULT_INTERVAL>
class ConnectionPool :
    public boost::noncopyable
{
public:
    ConnectionPool() :
        m_ConnectionCount(0),
        m_MaxConnection(MaxConn),
        m_IdleConnectionTimeout(IdleConnTimeout)
    {
    }

    int Attach(sockaddr_in& stAddr, TcpClientT** ppstClient)
    {
        typename ConnectionInfoMap::iterator iter = m_stConnectionMap.find(stAddr);
        if(iter == m_stConnectionMap.end())
        {
            ConnectionInfo<TcpClientT, TimerInterval>* pNewConnInfo = NewConnInfo(stAddr);
            if(pNewConnInfo == NULL)
                return -1;

            *ppstClient = &pNewConnInfo->stClient;
        }
        else
        {
            typename ConnectionInfoList::iterator listIter = iter->second.begin();
            if(listIter == iter->second.end())
            {
                ConnectionInfo<TcpClientT, TimerInterval>* pNewConnInfo = NewConnInfo(stAddr);
                if(pNewConnInfo == NULL)
                    return -1;

                *ppstClient = &pNewConnInfo->stClient;
            }
            else
            {
                ConnectionInfo<TcpClientT, TimerInterval>* pConnInfo = *listIter;
                iter->second.erase(listIter);

                PoolObject<Timer<ConnectionInfo<TcpClientT, TimerInterval>*, TimerInterval> >::Instance()
                    .Clear(pConnInfo->IdleConnTimerId);

                *ppstClient = &pConnInfo->stClient;
            }
        }
        return 0;
    }

    void Detach(TcpClientT* pstClient)
    {
        ConnectionInfo<TcpClientT, TimerInterval>* pConnInfo = 
            reinterpret_cast<ConnectionInfo<TcpClientT, TimerInterval>*>(pstClient);

        if(m_IdleConnectionTimeout != CONNECTIONPOOL_KEEPCONNECTION)
        {
            pConnInfo->IdleConnTimerId = PoolObject<Timer<ConnectionInfo<TcpClientT, TimerInterval>*, TimerInterval> >::Instance()
                .SetTimeout(this, m_IdleConnectionTimeout, pConnInfo);
        }

        typename ConnectionInfoMap::iterator iter = m_stConnectionMap.find(pConnInfo->stAddress);
        if(iter == m_stConnectionMap.end())
        {
            ConnectionInfoList stList;
            stList.push_back(pConnInfo);

            m_stConnectionMap.insert(std::make_pair(pConnInfo->stAddress, stList));
        }
        else
            iter->second.push_front(pConnInfo);
    }

    void Erase(TcpClientT* pstClient)
    {
        ConnectionInfo<TcpClientT, TimerInterval>* pConnInfo = 
            reinterpret_cast<ConnectionInfo<TcpClientT, TimerInterval>*>(pstClient);

        if(pConnInfo->stClient.IsConnected())
            pConnInfo->stClient.Disconnect();

        delete pConnInfo;
        --m_ConnectionCount;
    }

    void OnTimeout(ConnectionInfo<TcpClientT, TimerInterval>* pConnInfo)
    {
        if(pConnInfo->stClient.IsConnected())
            pConnInfo->stClient.Disconnect();

        typename ConnectionInfoMap::iterator iter = m_stConnectionMap.find(pConnInfo->stAddress);
        typename ConnectionInfoList::iterator listIter = iter->second.begin();
        for(; listIter != iter->second.end(); ++listIter)
        {
            if(*listIter == pConnInfo)
            {
                iter->second.erase(listIter);
                break;
            }
        }

        delete pConnInfo;
        if(m_ConnectionCount > 0)
            --m_ConnectionCount;
    }

    inline void SetMaxConnection(uint32_t max)
    {
        m_MaxConnection = max;
    }

    inline void SetIdleTimeout(uint32_t timeout)
    {
        m_IdleConnectionTimeout = timeout;
    }

    void Dump(std::string& strDump)
    {
        strDump.append((boost::format("Connection Count: %u\n") % m_ConnectionCount).str());
        strDump.append((boost::format("Max Connection Count: %u\n") % m_MaxConnection).str());
        strDump.append("---------------------------------------------------------\n");
        for(typename ConnectionInfoMap::iterator iter = m_stConnectionMap.begin();
            iter != m_stConnectionMap.end();
            ++iter)
        {
            strDump.append((boost::format("[%s:%d]:\n") % inet_ntoa(iter->first.sin_addr) % ntohs(iter->first.sin_port)).str());
            for(typename ConnectionInfoList::iterator listIter = iter->second.begin();
                listIter != iter->second.end();
                ++listIter)
            {
                strDump.append((boost::format("    Connect(sock:%d), object: 0x%llx\n")
                    % (*listIter)->stClient.m_ServerInterface.m_Channel.Socket
                    % (void*)(*listIter)).str());
            }
        }
    }

private:

    ConnectionInfo<TcpClientT, TimerInterval>* NewConnInfo(sockaddr_in& addr)
    {
        if(m_ConnectionCount >= m_MaxConnection)
            return NULL;

        ConnectionInfo<TcpClientT, TimerInterval>* pNewConnInfo = new ConnectionInfo<TcpClientT, TimerInterval>();
        memcpy(&pNewConnInfo->stAddress, &addr, sizeof(sockaddr_in));

        ++m_ConnectionCount;
        return pNewConnInfo;
    }

    uint32_t m_ConnectionCount;
    uint32_t m_MaxConnection;
    uint32_t m_IdleConnectionTimeout;

    typedef std::list<ConnectionInfo<TcpClientT, TimerInterval>*> ConnectionInfoList;
    typedef std::map<sockaddr_in, ConnectionInfoList, MemCompare<sockaddr_in> > ConnectionInfoMap;

    ConnectionInfoMap m_stConnectionMap;
};


#endif // define __CONNECTIONPOOL_HPP__
