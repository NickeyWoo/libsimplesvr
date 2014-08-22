/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-04-17
 *
--*/
#ifndef __KEEPCONNECTCLIENT_HPP__
#define __KEEPCONNECTCLIENT_HPP__

#include "TcpClient.hpp"
#include "Timer.hpp"

#ifndef KEEPCONNECTCLIENT_CONNECTTIMEOUT
    #define KEEPCONNECTCLIENT_CONNECTTIMEOUT 100
#endif

#ifndef KEEPCONNECTCLIENT_MAXRECONNECTINTERVAL
    #define KEEPCONNECTCLIENT_MAXRECONNECTINTERVAL  1000
#endif

template<uint32_t MaxReconnectInterval, uint32_t ConnectTimeout>
class KeepConnectPolicy
{
public:
    KeepConnectPolicy() :
        m_ConnectTimeout(ConnectTimeout)
    {
    }

    void OnConnected()
    {
        m_ConnectTimeout = ConnectTimeout;
    }

    void OnDisconnected()
    {
        m_ConnectTimeout = 0;
    }

    void OnMessage()
    {
    }

    void OnError()
    {
        if(m_ConnectTimeout == 0)
            m_ConnectTimeout = ConnectTimeout;
        else
        {
            m_ConnectTimeout *= 2;
            if(m_ConnectTimeout > MaxReconnectInterval)
                m_ConnectTimeout = MaxReconnectInterval;
        }
    }

    void OnTimeout()
    {
        if(m_ConnectTimeout == 0)
            m_ConnectTimeout = ConnectTimeout;
        else
        {
            m_ConnectTimeout *= 2;
            if(m_ConnectTimeout > MaxReconnectInterval)
                m_ConnectTimeout = MaxReconnectInterval;
        }
    }

    inline uint32_t GetTimeout()
    {
        return m_ConnectTimeout;
    }

private:
    uint32_t m_ConnectTimeout;
};

template<typename ServerImplT, typename ChannelDataT = void, 
            typename PolicyT = KeepConnectPolicy<KEEPCONNECTCLIENT_MAXRECONNECTINTERVAL, KEEPCONNECTCLIENT_CONNECTTIMEOUT>,
            int32_t TimerInterval = TIMER_DEFAULT_INTERVAL>
class KeepConnectClient :
    public TcpClient<ServerImplT, ChannelDataT>
{
public:

    int Connect(sockaddr_in& addr)
    {
        if(this->m_ServerInterface.m_Channel.Socket == -1)
            return -1;

        memcpy(&this->m_ServerInterface.m_Channel.Address, &addr, sizeof(sockaddr_in));
        if(connect(this->m_ServerInterface.m_Channel.Socket, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1 && 
            errno != EINPROGRESS)
        {
            close(this->m_ServerInterface.m_Channel.Socket);
            this->m_ServerInterface.m_Channel.Socket = -1;
            return -1;
        }
        return 0;
    }

    void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
    {
        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
        if(scheduler.Update(this, EventScheduler::PollType::POLLIN) == -1)
            throw InternalException((boost::format("[%s:%d][error] epoll_ctl update sockfd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

        PoolObject<Timer<void, TimerInterval> >::Instance().Clear(m_TimeoutID);
        m_Policy.OnConnected();

        LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] connected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

        this->OnConnected(pInterface->m_Channel);

        LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] connected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());
    }

    void OnReadable(ServerInterface<ChannelDataT>* pInterface)
    {
        char buffer[65535];
        IOBuffer in(buffer, 65535);
        pInterface->m_Channel >> in;

        if(in.GetReadSize() == 0)
        {
            EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
            scheduler.UnRegister(this);

            this->Disconnect();

            LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] disconnected process.") %
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

            this->OnDisconnected(pInterface->m_Channel);

            LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] disconnected process.") %
                                    inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                    ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

            m_Policy.OnDisconnected();
            m_TimeoutID = PoolObject<Timer<void, TimerInterval> >::Instance().SetTimeout(
                                boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, PolicyT,
                                            TimerInterval>::OnConnectTimeout, this), 
                                m_Policy.GetTimeout());
        }
        else
        {
            m_Policy.OnMessage();

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
        EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
        scheduler.UnRegister(this);

        this->Disconnect();

        LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] disconnected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

        this->OnError(pInterface->m_Channel);

        LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] disconnected process.") %
                                inet_ntoa(pInterface->m_Channel.Address.sin_addr) %
                                ntohs(pInterface->m_Channel.Address.sin_port)).str().c_str());

        PoolObject<Timer<void, TimerInterval> >::Instance().Clear(m_TimeoutID);

        m_Policy.OnError();
        m_TimeoutID = PoolObject<Timer<void> >::Instance().SetTimeout(
                            boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, PolicyT,
                                        TimerInterval>::OnConnectTimeout, this), 
                            m_Policy.GetTimeout());
    }

    void OnConnectTimeout()
    {
        if(!this->IsConnected())
        {
            PoolObject<EventScheduler>::Instance().UnRegister(this);
            this->Disconnect();

            this->Reconnect();
            PoolObject<EventScheduler>::Instance().Register(this, EventScheduler::PollType::POLLOUT);

            m_Policy.OnTimeout();
            m_TimeoutID = PoolObject<Timer<void, TimerInterval> >::Instance().SetTimeout(
                            boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, PolicyT,
                                        TimerInterval>::OnConnectTimeout, this), 
                            m_Policy.GetTimeout());
        }
    }

    KeepConnectClient()
    {
        if(this->m_ServerInterface.m_Channel.Socket == -1)
            return;

        int keepalive = 1;
        if(setsockopt(this->m_ServerInterface.m_Channel.Socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int)) == -1)
        {
            close(this->m_ServerInterface.m_Channel.Socket);
            this->m_ServerInterface.m_Channel.Socket = -1;
            return;
        }
    }

protected:
    PolicyT m_Policy;
    typename Timer<void, TimerInterval>::TimerID m_TimeoutID;
};



#endif // define __KEEPCONNECTCLIENT_HPP__
