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

class UdpService :
    public UdpServer<UdpService>
{
public:
    void OnMessage(ChannelType& channel, IOBuffer& in)
    {
        LOG("[PID:%u][%s:%d] client message", 
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
        if(!RegisterUdpServer<UdpService>("server_interface"))
            return false;

        return true;
    }

};

AppRun(MyApp);







