/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-14
 *
--*/
#ifndef __TCPCLIENT_HPP__
#define __TCPCLIENT_HPP__

#include <netinet/tcp.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"
#include "Server.hpp"
#include "PoolObject.hpp"
#include "Pool.hpp"
#include "Timer.hpp"
#include "EventScheduler.hpp"
#include "Clock.hpp"

template<typename ServerImplT, typename ChannelDataT = void, uint32_t CacheSize = 65535>
class TcpClient
{
public:
    typedef Channel<ChannelDataT> ChannelType;

    int Connect(sockaddr_in& addr)
    {
        if(m_ServerInterface.m_Channel.Socket == -1)
        {
#ifdef __USE_GNU
            m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
            if(m_ServerInterface.m_Channel.Socket == -1)
                return -1;
#else
            m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM, 0);
            if(m_ServerInterface.m_Channel.Socket == -1)
                return -1;

            if(SetCloexecFd(m_ServerInterface.m_Channel.Socket) < 0)
            {
                close(m_ServerInterface.m_Channel.Socket);
                m_ServerInterface.m_Channel.Socket = -1;
                return -1;
            }
#endif
        }

        memcpy(&m_ServerInterface.m_Channel.Address, &addr, sizeof(sockaddr_in));
        if(connect(m_ServerInterface.m_Channel.Socket, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1)
            return -1;

        this->OnConnected(m_ServerInterface.m_Channel);

        if(Pool::Instance().IsStartup())
        {
            EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
            return scheduler.Register(this, EventScheduler::PollType::POLLIN);
        }
        else
            return 0;
    }

    int Connect(sockaddr_in& addr, timeval* timeout)
    {
        if(timeout == NULL)
            return -1;

        if(m_ServerInterface.m_Channel.Socket == -1)
        {
#ifdef __USE_GNU
            m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0);
            if(m_ServerInterface.m_Channel.Socket == -1)
                return -1;
#else
            m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM, 0);
            if(m_ServerInterface.m_Channel.Socket == -1)
                return -1;

            if(SetCloexecFd(m_ServerInterface.m_Channel.Socket, FD_CLOEXEC | O_NONBLOCK) < 0)
            {
                close(m_ServerInterface.m_Channel.Socket);
                m_ServerInterface.m_Channel.Socket = -1;
                return -1;
            }
#endif
        }

        memcpy(&m_ServerInterface.m_Channel.Address, &addr, sizeof(sockaddr_in));
        if(connect(m_ServerInterface.m_Channel.Socket, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1 && 
           errno != EINPROGRESS)
            return -1;

        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
        if(Pool::Instance().IsStartup())
        {
            m_AsyncConnectTimerId = PoolObject<Timer<void> >::Instance().SetTimeout(
                                        boost::bind(&ServerImplT::OnAsyncConnectTimout, this), 
                                        (timeout->tv_sec*1000 + timeout->tv_usec/1000));
            return scheduler.Register(this, EventScheduler::PollType::POLLOUT);
        }
        else
        {
            int iRet = scheduler.Wait(&m_ServerInterface, EventScheduler::PollType::POLLOUT, timeout);
            if(iRet == -1)
                return -1;

            if(iRet == 0)
            {
                this->OnConnectTimout(m_ServerInterface.m_Channel);
                Disconnect();
            }
            else
            {
                int errinfo;
                socklen_t errlen;
                if(-1 == getsockopt(m_ServerInterface.m_Channel.Socket, SOL_SOCKET, SO_ERROR, &errinfo, &errlen) ||
                   errinfo != 0)
                {
                    this->OnConnectTimout(m_ServerInterface.m_Channel);
                    Disconnect();
                }
                else
                    this->OnConnected(m_ServerInterface.m_Channel);
            }
        }
        return 0;
    }

    inline void Close()
    {
        close(m_ServerInterface.m_Channel.Socket);
        m_ServerInterface.m_Channel.Socket = -1;
    }

    void Disconnect()
    {
        if(m_AsyncConnectTimerId != 0)
        {
            PoolObject<Timer<void> >::Instance().Clear(m_AsyncConnectTimerId);
            m_AsyncConnectTimerId = 0;
        }

        this->OnDisconnected(m_ServerInterface.m_Channel);

        if(Pool::Instance().IsStartup())
            PoolObject<EventScheduler>::Instance().UnRegister(this);

        Shutdown(SHUT_RDWR);
        Close();
    }

    void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
    {
        int errinfo;
        socklen_t errlen;
        if(-1 == getsockopt(pInterface->m_Channel.Socket, SOL_SOCKET, SO_ERROR, &errinfo, &errlen) ||
           errinfo != 0)
        {
            this->OnConnectTimout(pInterface->m_Channel);
            Disconnect();
        }
        else
        {
            PoolObject<Timer<void> >::Instance().Clear(m_AsyncConnectTimerId);
            m_AsyncConnectTimerId = 0;

            this->OnConnected(pInterface->m_Channel);
            PoolObject<EventScheduler>::Instance().Update(this, EventScheduler::PollType::POLLIN);
        }
    }

    void OnReadable(ServerInterface<ChannelDataT>* pInterface)
    {
        int dwRemainSize = CacheSize - m_dwAvailableSize;
        if(dwRemainSize <= 0)
        {
            Disconnect();

            throw InternalException((boost::format("[%s:%d][error] package cache is full.") % __FILE__ % __LINE__).str().c_str());
        }

        IOBuffer ioRecvBuffer(&m_cPackageCache[m_dwAvailableSize], dwRemainSize);
        pInterface->m_Channel >> ioRecvBuffer;

        if(ioRecvBuffer.GetReadSize() == 0)
        {
            Disconnect();
        }
        else
        {
            m_dwAvailableSize += ioRecvBuffer.GetReadSize();

            IOBuffer in(m_cPackageCache, m_dwAvailableSize, m_dwAvailableSize);
            this->OnMessage(pInterface->m_Channel, in);

            m_dwAvailableSize = in.GetReadSize() - in.GetReadPosition();
            if(in.GetReadPosition() > 0 && m_dwAvailableSize > 0)
                memmove(m_cPackageCache, &m_cPackageCache[in.GetReadPosition()], m_dwAvailableSize);
        }
    }

    void OnErrorable(ServerInterface<ChannelDataT>* pInterface)
    {
        this->OnError(pInterface->m_Channel);
        Disconnect();
    }

    // tcp client interface
    TcpClient() :
        m_dwAvailableSize(0),
        m_AsyncConnectTimerId(0)
    {
        m_ServerInterface.m_Channel.Socket = -1;

        m_ServerInterface.m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, reinterpret_cast<ServerImplT*>(this), _1);
        m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, reinterpret_cast<ServerImplT*>(this), _1);
        m_ServerInterface.m_ErrorCallback = boost::bind(&ServerImplT::OnErrorable, reinterpret_cast<ServerImplT*>(this), _1);
    }

    virtual ~TcpClient()
    {
    }

    virtual void OnConnectTimout(ChannelType& channel)
    {
    }

    virtual void OnConnected(ChannelType& channel)
    {
    }

    virtual void OnError(ChannelType& channel)
    {
    }

    virtual void OnDisconnected(ChannelType& channel)
    {
    }

    virtual void OnMessage(ChannelType& channel, IOBuffer& in)
    {
    }

    bool IsConnected()
    {
        int fd = m_ServerInterface.m_Channel.Socket;
        if(fd == -1)
            return false;

        struct tcp_info info;
        int len = sizeof(struct tcp_info);
        if(-1 == getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len))
            return false;

        if(info.tcpi_state == TCP_ESTABLISHED)
            return true;

        return false;
    }

    inline int Reconnect()
    {
        return Reconnect(m_ServerInterface.m_Channel.Address);
    }

    int Reconnect(sockaddr_in& addr)
    {
        Disconnect();

        if(Connect(addr) != 0)
            return -1;

        return 0;
    }

    inline ssize_t Send(IOBuffer& out)
    {
        return send(m_ServerInterface.m_Channel.Socket, 
                    out.GetWriteBuffer(), out.GetWriteSize(), 0);
    }

    inline ssize_t Recv(IOBuffer& in)
    {
        m_ServerInterface.m_Channel >> in;
        return in.GetReadSize();
    }

    inline int Shutdown(int how)
    {
        return shutdown(m_ServerInterface.m_Channel.Socket, how);
    }

    void OnAsyncConnectTimout()
    {
        m_AsyncConnectTimerId = 0;
        this->OnConnectTimout(m_ServerInterface.m_Channel);
        Disconnect();
    }

    ServerInterface<ChannelDataT>   m_ServerInterface;
    uint32_t                        m_dwAvailableSize;
    char                            m_cPackageCache[CacheSize];
    typename Timer<void>::TimerID   m_AsyncConnectTimerId;
};


#endif // define __TCPCLIENT_HPP__
