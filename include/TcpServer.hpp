/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-01
 *
--*/
#ifndef __TCPSERVER_HPP__
#define __TCPSERVER_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"
#include "Server.hpp"
#include "EventScheduler.hpp"
#include "Clock.hpp"

#define DEFAULT_SOCK_BACKLOG    100

template<typename ServerImplT, typename ChannelDataT = void, uint32_t CacheSize = 65535>
class TcpServer
{
public:
    typedef Channel<ChannelDataT> ChannelType;

    int Listen(sockaddr_in& addr)
    {
        if(m_ServerInterface.m_Channel.Socket == -1)
            return -1;

        if(-1 == bind(m_ServerInterface.m_Channel.Socket, (sockaddr*)&addr, sizeof(sockaddr_in)))
        {
            close(m_ServerInterface.m_Channel.Socket);
            m_ServerInterface.m_Channel.Socket = -1;
            return -1;
        }

        if(-1 == listen(m_ServerInterface.m_Channel.Socket, DEFAULT_SOCK_BACKLOG))
        {
            close(m_ServerInterface.m_Channel.Socket);
            m_ServerInterface.m_Channel.Socket = -1;
            return -1;
        }

        return 0;
    }

    void OnAcceptable(ServerInterface<ChannelDataT>* pInterface)
    {
        sockaddr_in cliAddr;
        bzero(&cliAddr, sizeof(sockaddr_in));
        socklen_t len = sizeof(sockaddr_in);

#ifdef __USE_GNU
        int clifd = accept4(pInterface->m_Channel.Socket, (sockaddr*)&cliAddr, &len, SOCK_CLOEXEC);
        if(clifd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
        }
#else
        int clifd = accept(pInterface->m_Channel.Socket, (sockaddr*)&cliAddr, &len);
        if(clifd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
        }

        if(SetCloexecFd(clifd) < 0)
        {
            close(clifd);
            throw InternalException((boost::format("[%s:%d][error] SetNonblockAndCloexecFd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
        }
#endif
        ServerInterface<ChannelDataT>* pChannelInterface = new ServerInterface<ChannelDataT>();
        pChannelInterface->m_Channel.Socket = clifd;
        memcpy(&pChannelInterface->m_Channel.Address, &cliAddr, sizeof(sockaddr_in));

        pChannelInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, reinterpret_cast<ServerImplT*>(this), _1);
        pChannelInterface->m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, reinterpret_cast<ServerImplT*>(this), _1);
        pChannelInterface->m_ErrorCallback = boost::bind(&ServerImplT::OnErrorable, reinterpret_cast<ServerImplT*>(this), _1);

        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
        if(scheduler.Register(pChannelInterface, EventScheduler::PollType::POLLIN) == -1)
        {
            shutdown(pChannelInterface->m_Channel.Socket, SHUT_RDWR);
            close(pChannelInterface->m_Channel.Socket);
            delete pChannelInterface;
            throw InternalException((boost::format("[%s:%d][error] epoll_ctl add new sockfd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
        }

        this->OnConnected(pChannelInterface->m_Channel);
    }

    void OnReadable(ServerInterface<ChannelDataT>* pInterface)
    {
        int dwRemainSize = CacheSize - m_dwAvailableSize;
        if(dwRemainSize <= 0)
        {
            this->DisconnectClient(pInterface->m_Channel);

            throw InternalException((boost::format("[%s:%d][error] package cache is full.") % __FILE__ % __LINE__).str().c_str());
        }

        IOBuffer ioRecvBuffer(&m_cPackageCache[m_dwAvailableSize], dwRemainSize);
        pInterface->m_Channel >> ioRecvBuffer;

        if(ioRecvBuffer.GetReadSize() == 0)
        {
            this->OnDisconnected(pInterface->m_Channel);

            EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
            scheduler.UnRegister(pInterface);
            shutdown(pInterface->m_Channel.Socket, SHUT_RDWR);
            close(pInterface->m_Channel.Socket);
            delete pInterface;
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

    void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
    {
    }

    void OnErrorable(ServerInterface<ChannelDataT>* pInterface)
    {
        this->OnError(pInterface->m_Channel);
        this->DisconnectClient(pInterface->m_Channel);
    }

    // udp server interface
    TcpServer() :
        m_dwAvailableSize(0)
    {
#ifdef __USE_GNU
        m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
        if(m_ServerInterface.m_Channel.Socket == -1)
            return;
#else
        m_ServerInterface.m_Channel.Socket = socket(PF_INET, SOCK_STREAM, 0);
        if(m_ServerInterface.m_Channel.Socket == -1)
            return;

        if(SetCloexecFd(m_ServerInterface.m_Channel.Socket) < 0)
        {
            close(m_ServerInterface.m_Channel.Socket);
            m_ServerInterface.m_Channel.Socket = -1;
            return;
        }
#endif

        int reuse = 1;
        if(-1 == setsockopt(m_ServerInterface.m_Channel.Socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
        {
            close(m_ServerInterface.m_Channel.Socket);
            m_ServerInterface.m_Channel.Socket = -1;
            return;
        }

        m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnAcceptable, this, _1);
    }

    virtual ~TcpServer()
    {
    }

    virtual void OnConnected(ChannelType& channel)
    {
    }

    virtual void OnDisconnected(ChannelType& channel)
    {
    }

    virtual void OnMessage(ChannelType& channel, IOBuffer& in)
    {
    }

    virtual void OnError(ChannelType& channel)
    {
    }

    void DisconnectClient(ChannelType& channel)
    {
        this->OnDisconnected(channel);

        ServerInterface<ChannelDataT>* pChannelInterface = GetServerInterface(&channel);
        PoolObject<EventScheduler>::Instance().UnRegister(pChannelInterface);
        shutdown(pChannelInterface->m_Channel.Socket, SHUT_RDWR);
        close(pChannelInterface->m_Channel.Socket);
        delete pChannelInterface;
    }

    ServerInterface<ChannelDataT>   m_ServerInterface;
    uint32_t                        m_dwAvailableSize;
    char                            m_cPackageCache[CacheSize];
};

#endif // define __TCPSERVER_HPP__
