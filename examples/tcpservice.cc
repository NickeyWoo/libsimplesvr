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

#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"
#include "hashtable.hpp"

#include "CharsetConversion.hpp"
#include "Configure.hpp"
#include "PoolObject.hpp"
#include "Pool.hpp"
#include "Channel.hpp"
#include "IOBuffer.hpp"
#include "Timer.hpp"
#include "TcpServer.hpp"
#include "UdpServer.hpp"
#include "TcpClient.hpp"
#include "KeepConnectClient.hpp"
#include "LoadBalance.hpp"
#include "ConnectionPool.hpp"
#include "Application.hpp"

struct TimerValue {
    uint32_t dwValue;
};

class TcpService :
    public TcpServer<TcpService>
{
public:
    void OnMessage(ChannelType& channel, IOBuffer& in)
    {
        LOG("[PID:%u][%s:%d] client message", 
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));

        in.ReadSeek(in.GetReadSize());
    }

    void OnConnected(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] client connected.",
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));
    }

    void OnDisconnected(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] disconnect client.",
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));
    }

    void OnMyTimeout(TimerValue* pVal)
    {
        ++pVal->dwValue;

        PoolObject<Timer<TimerValue*> >::Instance().SetTimeout(
            boost::bind(&TcpService::OnMyTimeout, this, _1), 
            5000, pVal);
    }

};

class MyApp :
    public Application<MyApp>
{
public:

    bool Initialize(int argc, char* argv[])
    {
        if(!RegisterTcpServer(m_stTcpService, "server_interface"))
            return false;

        PoolObject<Timer<void> >::Instance().SetTimeout(this, 1000);

        m_stValue.dwValue = 0x1234;
        PoolObject<Timer<TimerValue*> >::Instance().SetTimeout(
            boost::bind(&TcpService::OnMyTimeout, &m_stTcpService, _1), 
            5000, &m_stValue);

        Pool::Instance().RegisterStartupCallback(boost::bind(&MyApp::OnPoolStartup, this));

        printf("%s\n", __FUNCTION__);
        return true;
    }

    bool OnPoolStartup()
    {
        if(Pool::Instance().GetID() == 1)
            printf("%s\n", __FUNCTION__);
        else
            printf("abc\n");
        return true;
    }

    void OnTimeout()
    {
        printf("time:%u\n", (uint32_t)time(NULL));

        PoolObject<Timer<void> >::Instance().SetTimeout(this, 1000);
    }

    TimerValue m_stValue;

    TcpService m_stTcpService;
};

AppRun(MyApp);







