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
#include "EventScheduler.hpp"
#include "Clock.hpp"

template<typename ServerImplT, typename ChannelDataT = void>
class TcpClient
{
public:
    typedef Channel<ChannelDataT> ChannelType;

    int Connect(sockaddr_in& addr)
    {
        if(m_ServerInterface.m_Channel.Socket == -1)
            return -1;

        memcpy(&m_ServerInterface.m_Channel.Address, &addr, sizeof(sockaddr_in));
        if(connect(m_ServerInterface.m_Channel.Socket, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1)
        {
            close(m_ServerInterface.m_Channel.Socket);
            m_ServerInterface.m_Channel.Socket = -1;
            return -1;
        }

        LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] connected process.") %
                                inet_ntoa(addr.sin_addr) %
                                ntohs(addr.sin_port)).str().c_str());

        this->OnConnected(m_ServerInterface.m_Channel);

        LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] connected process.") %
                                inet_ntoa(addr.sin_addr) %
                                ntohs(addr.sin_port)).str().c_str());

        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
        return scheduler.Register(this, EventScheduler::PollType::POLLIN);
    }

    inline void Close()
    {
        close(m_ServerInterface.m_Channel.Socket);
        m_ServerInterface.m_Channel.Socket = -1;
    }

    void Disconnect()
    {
        PoolObject<EventScheduler>::Instance().UnRegister(this);
        Shutdown(SHUT_RDWR);
    }

    void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
    {
    }

    void OnReadable(ServerInterface<ChannelDataT>* pInterface)
    {
        char buffer[65535];
        IOBuffer in(buffer, 65535);
        pInterface->m_Channel >> in;

        if(in.GetReadSize() == 0)
        {
            Disconnect();

            LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] disconnected process.") %
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

            this->OnDisconnected(pInterface->m_Channel);

            LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] disconnected process.") %
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());
        }
        else
        {
            LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] message process.") % 
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

            this->OnMessage(pInterface->m_Channel, in);

            LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] message process.") %
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) % 
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());
        }
    }

    void OnErrorable(ServerInterface<ChannelDataT>* pInterface)
    {
        Disconnect();

        LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] disconnected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

        this->OnError(pInterface->m_Channel);

        LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] disconnected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());
    }

    // tcp client interface
    TcpClient()
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

        m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, reinterpret_cast<ServerImplT*>(this), _1);
        m_ServerInterface.m_ErrorCallback = boost::bind(&ServerImplT::OnErrorable, reinterpret_cast<ServerImplT*>(this), _1);
    }

    virtual ~TcpClient()
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
        if(IsConnected())
            return 0;

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

    ServerInterface<ChannelDataT> m_ServerInterface;
};


#endif // define __TCPCLIENT_HPP__
