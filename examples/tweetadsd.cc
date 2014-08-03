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
#include "Application.hpp"

struct TweetADS {
	uint64_t	tweetid;
	uint16_t	adnum;
	uint64_t	adbuffer[10];
} __attribute__((packed));

TimerHashTable<uint64_t, TweetADS> g_HashTable;

struct Request {
	uint64_t tweetid;
};

class tweetadsd :
//	public UdpServer<tweetadsd>
	public TcpServer<tweetadsd>
{
public:
	void OnMessage(ChannelType& channel, IOBuffer& in)
	{
	/*
		IOBuffer out;

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
	*/
	}

	void OnConnected(ChannelType& channel)
	{
		LOG("[PID:%u][%s:%d] client connected.", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}

	void OnDisconnected(ChannelType& channel)
	{
		LOG("[PID:%u][%s:%d] disconnect client.", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}
};

class Client :
	public KeepConnectClient<Client>
{
public:

	void OnMessage(ChannelType& channel, IOBuffer& in)
	{
		std::string inbuf;
		in.Dump(inbuf);
		LOG("[PID:%u][%s:%d] say: %s", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port), inbuf.c_str());
	}

	void OnConnected(ChannelType& channel)
	{
		LOG("[PID:%u][%s:%d] connect server success.", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}

	void OnError(ChannelType& channel)
	{
		LOG("[PID:%u][%s:%d] connect server error.", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}

	void OnDisconnected(ChannelType& channel)
	{
		LOG("[PID:%u][%s:%d] disconnect server.", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}
};

class MyApp :
	public Application<MyApp>
{
public:

	bool Initialize(int argc, char* argv[])
	{
		LoadBalance<RoutePolicy> lb;
		if(!lb.LoadConfigure("gateway.conf"))
		{
			printf("error: initialize load balance fail.\n");
			return false;
		}

		std::map<uint16_t, uint64_t> m;
		int count = atoi(argv[2]);
		while(count > 0)
		{
			sockaddr_in addr;
			bzero(&addr, sizeof(sockaddr_in));

			lb.Route(&addr);

			++m[ntohs(addr.sin_port)];

			if(count == 20000)
			{
				addr.sin_port = htons(5600);
				lb.Failure(&addr);
				printf("Fail: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
				lb.ShowInformation();
			}
			else
				lb.Success(&addr);
			--count;
			usleep(1000);
		}
		lb.ShowInformation();

		for(std::map<uint16_t, uint64_t>::iterator iter = m.begin(); iter != m.end(); ++iter)
		{
			printf("%d: %lu\n", iter->first, iter->second);
		}

		return false;

		if(argc == 2 && !RegisterTcpServer(m_tweetadsd, "server_interface"))
			return false;
		else if(argc == 3 && !RegisterTcpClient<Client>("client_interface"))
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







