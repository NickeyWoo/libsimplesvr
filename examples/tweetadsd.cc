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

struct TweetADS {
    uint64_t    tweetid;
    uint16_t    adnum;
    uint64_t    adbuffer[10];
} __attribute__((packed));

TimerHashTable<uint64_t, TweetADS> g_HashTable;

struct Request {
    uint64_t tweetid;
};

struct Status
{
    uint64_t a;
    uint64_t b;
};

class tweetadsd :
    public TcpServer<tweetadsd, Status>
{
public:
    void OnMessage(ChannelType& channel, IOBuffer& in)
    {
        LOG("[PID:%u][%s:%d] client connected.", Pool::Instance().GetID(), inet_ntoa(channel.Address.sin_addr), ntohs(channel.Address.sin_port));
        char buffer[65535];
        bzero(buffer, 65535);
        IOBuffer out(buffer, 65535);

        Request request;
        bzero(&request, sizeof(Request));

        in >> request.tweetid;
        out << request.tweetid;

        TweetADS* pValue = g_HashTable.Hash(request.tweetid);
        if(!pValue)
        {
            uint16_t num = 0;
            out << num;
        }
        else
        {
            out << pValue->adnum;
            for(uint16_t i=0; i<pValue->adnum; ++i)
                out << pValue->adbuffer[i];
        }

        PoolObject<Timer<void> >::Instance().SetTimeout(this, 100);
        channel << out;
    }

    void OnConnected(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] client connected.", Pool::Instance().GetID(), inet_ntoa(channel.Address.sin_addr), ntohs(channel.Address.sin_port));
    }

    void OnDisconnected(ChannelType& channel)
    {
        LOG("[PID:%u][%s:%d] disconnect client.", Pool::Instance().GetID(), inet_ntoa(channel.Address.sin_addr), ntohs(channel.Address.sin_port));
    }

    void OnTimeout()
    {
    }
};

class MyApp :
    public Application<MyApp>
{
public:

    bool Initialize(int argc, char* argv[])
    {
        if(!RegisterTcpServer(m_tweetadsd, "server_interface"))
            return false;

        // other initialize
        std::map<std::string, std::string> stStorageConfig = Configure::Get("storage");
        Seed seed(atoi(stStorageConfig["seed"].c_str()), atoi(stStorageConfig["count"].c_str()));

        SharedMemoryStorage sms;
        if(SharedMemoryStorage::OpenStorage(&sms, 
                        strtoul(stStorageConfig["shmkey"].c_str(), NULL, 16),
                        TimerHashTable<uint64_t, TweetADS>::GetBufferSize(seed)) < 0)
        {
            printf("error: open hashtable storage fail.\n");
            return false;
        }

        g_HashTable = TimerHashTable<uint64_t, TweetADS>::LoadHashTable(sms, seed);
        g_HashTable.SetDefaultTimeout((time_t)strtoul(stStorageConfig["timeout"].c_str(), NULL, 10));

        return true;
    }

    tweetadsd m_tweetadsd;
};

AppRun(MyApp);







