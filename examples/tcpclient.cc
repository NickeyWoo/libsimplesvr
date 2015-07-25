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

class MyTcpClient :
    public TcpClient<MyTcpClient>
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

    void OnConnectTimout(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] client connect timeout.",
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));
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
};

class MyApp :
    public Application<MyApp>
{
public:

    bool Initialize(int argc, char* argv[])
    {
        Pool::Instance().RegisterStartupCallback(boost::bind(&MyApp::OnPoolStartup, this));

        return true;
    }

    bool OnPoolStartup()
    {
        sockaddr_in addr;
        bzero(&addr, sizeof(sockaddr_in));
        addr.sin_family = PF_INET;
        addr.sin_addr.s_addr = inet_addr("0.0.0.0");
        addr.sin_port = htons(1234);

        timeval tv;
        bzero(&tv, sizeof(timeval));
        tv.tv_usec = 200000;

        m_Client.Connect(addr, &tv);
        return true;
    }

    MyTcpClient m_Client;
};

AppRun(MyApp);







