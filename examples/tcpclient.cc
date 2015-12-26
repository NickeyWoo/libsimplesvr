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

class MyTcpClient :
    public TcpClient<MyTcpClient>
{
public:
    void OnMessage(ChannelType& channel, IOBuffer& in)
    {
        LOG("[PID:%u][%s:%d] server message: %s", 
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port), in.GetReadBuffer());

        in.ReadSeek(in.GetReadSize());

        PoolObject<ConnectionPool<MyTcpClient> >::Instance().Detach(this);
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

        SendRequest();
    }

    void OnDisconnected(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] disconnect client.",
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));
    }

    void SendRequest()
    {
        std::string str = (boost::format("time:%lu") % time(NULL)).str();
        LOG("send request: %s", str.c_str());

        char buffer[4096];
        IOBuffer out(buffer, 4096);
        out.Write(str.c_str(), str.length());

        this->Send(out);
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

    void OnTimeout()
    {
        //PoolObject<Timer<void> >::Instance().SetTimeout(this, 5000);

        std::map<std::string, std::string>& stClientConfig = Configure::Get("client_interface");

        sockaddr_in addr;
        bzero(&addr, sizeof(sockaddr_in));
        addr.sin_family = PF_INET;
        addr.sin_port = htons(atoi(stClientConfig["port"].c_str()));
        addr.sin_addr.s_addr = inet_addr(stClientConfig["ip"].c_str());

        MyTcpClient* pClient = NULL;
        ConnectionPool<MyTcpClient>& stPool = PoolObject<ConnectionPool<MyTcpClient> >::Instance();
        if(0 != stPool.Attach(addr, &pClient) || pClient == NULL)
        {
            fprintf(stderr, "error: attach connection fail.\n");
            return;
        }

        if(!pClient->IsConnected())
        {
            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;

            if(0 != pClient->Connect(addr, &tv))
            {
                fprintf(stderr, "error: connect server fail.\n");
            }
            return;
        }

        pClient->SendRequest();
    }

    bool OnPoolStartup()
    {
        // 10s
        PoolObject<ConnectionPool<MyTcpClient> >::Instance().SetIdleTimeout(5000);

        PoolObject<Timer<void> >::Instance().SetTimeout(this, 4000);
        return true;
    }

};

AppRun(MyApp);







