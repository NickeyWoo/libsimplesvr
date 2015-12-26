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

class TcpService :
    public TcpServer<TcpService>
{
public:
    void OnMessage(ChannelType& channel, IOBuffer& in)
    {
        TRACE_LOG("[PID:%u][%s:%d] client message: %s", 
                  Pool::Instance().GetID(),
                  inet_ntoa(channel.Address.sin_addr),
                  ntohs(channel.Address.sin_port), 
                  in.GetReadBuffer());

        char buffer[4096];
        IOBuffer out(buffer, 4096);
        out.Write("resp => ", 8);
        out.Write(in.GetReadBuffer(), in.GetReadSize());
        
        channel << out;

        in.ReadSeek(in.GetReadSize());
    }

    void OnConnected(ChannelType& channel)
    {
        TRACE_LOG("[PID:%u][%s:%d] client connected.",
                Pool::Instance().GetID(),
                inet_ntoa(channel.Address.sin_addr),
                ntohs(channel.Address.sin_port));
    }

    void OnDisconnected(ChannelType& channel)
    {
        TRACE_LOG("[PID:%u][%s:%d] disconnect client.",
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
        if(!RegisterTcpServer(m_stTcpService, "server_interface"))
            return false;

        return true;
    }

    TcpService m_stTcpService;
};

AppRun(MyApp);







