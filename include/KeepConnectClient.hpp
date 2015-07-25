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

template<typename ServerImplT, typename ChannelDataT = void, uint32_t CacheSize = 65535,
            typename PolicyT = KeepConnectPolicy<KEEPCONNECTCLIENT_MAXRECONNECTINTERVAL, KEEPCONNECTCLIENT_CONNECTTIMEOUT>,
            int32_t TimerInterval = TIMER_DEFAULT_INTERVAL>
class KeepConnectClient :
    public TcpClient<ServerImplT, ChannelDataT, CacheSize>
{
public:
    typedef Channel<ChannelDataT> ChannelType;

    int Connect(sockaddr_in& addr)
    {
        if(TcpClient<ServerImplT, ChannelDataT>::Connect(addr) != 0)
            return -1;

        int keepalive = 1;
        if(setsockopt(this->m_ServerInterface.m_Channel.Socket, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int)) == -1)
        {
            this->Disconnect();
            return -1;
        }
        return 0;
    }

    void OnReadable(ServerInterface<ChannelDataT>* pInterface)
    {
        int dwRemainSize = CacheSize - this->m_dwAvailableSize;
        if(dwRemainSize <= 0)
        {
            this->Disconnect();

            m_Policy.OnDisconnected();
            m_TimeoutID = PoolObject<Timer<void, TimerInterval> >::Instance().SetTimeout(
                                boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, CacheSize,
                                             PolicyT, TimerInterval>::OnConnectTimeout, this), 
                                m_Policy.GetTimeout());

            throw InternalException((boost::format("[%s:%d][error] package cache is full.") % __FILE__ % __LINE__).str().c_str());
        }

        IOBuffer ioRecvBuffer(&this->m_cPackageCache[this->m_dwAvailableSize], dwRemainSize);
        pInterface->m_Channel >> ioRecvBuffer;

        if(ioRecvBuffer.GetReadSize() == 0)
        {
            this->Disconnect();

            m_Policy.OnDisconnected();
            m_TimeoutID = PoolObject<Timer<void, TimerInterval> >::Instance().SetTimeout(
                                boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, CacheSize,
                                             PolicyT, TimerInterval>::OnConnectTimeout, this), 
                                m_Policy.GetTimeout());
        }
        else
        {
            this->m_dwAvailableSize += ioRecvBuffer.GetReadSize();

            IOBuffer in(this->m_cPackageCache, this->m_dwAvailableSize, this->m_dwAvailableSize);
            this->OnMessage(pInterface->m_Channel, in);

            this->m_dwAvailableSize = in.GetReadSize() - in.GetReadPosition();
            if(in.GetReadPosition() > 0 && this->m_dwAvailableSize > 0)
                memmove(this->m_cPackageCache, &this->m_cPackageCache[in.GetReadPosition()], this->m_dwAvailableSize);
        }
    }

    void OnErrorable(ServerInterface<ChannelDataT>* pInterface)
    {
        this->OnError(pInterface->m_Channel);
        this->Disconnect();

        m_Policy.OnError();
        PoolObject<Timer<void, TimerInterval> >::Instance().Clear(m_TimeoutID);
        m_TimeoutID = PoolObject<Timer<void> >::Instance().SetTimeout(
                            boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, CacheSize, 
                                        PolicyT, TimerInterval>::OnConnectTimeout, this), 
                            m_Policy.GetTimeout());
    }

    void OnConnectTimeout()
    {
        if(!this->IsConnected() && this->Reconnect() != 0)
        {
            m_Policy.OnTimeout();
            m_TimeoutID = PoolObject<Timer<void, TimerInterval> >::Instance().SetTimeout(
                            boost::bind(&KeepConnectClient<ServerImplT, ChannelDataT, CacheSize, 
                                        PolicyT, TimerInterval>::OnConnectTimeout, this), 
                            m_Policy.GetTimeout());
        }
    }

    KeepConnectClient()
    {
    }

protected:
    PolicyT m_Policy;
    typename Timer<void, TimerInterval>::TimerID m_TimeoutID;
};



#endif // define __KEEPCONNECTCLIENT_HPP__
