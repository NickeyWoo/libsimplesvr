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
#include "Pool.hpp"
#include "Channel.hpp"
#include "IOBuffer.hpp"
#include "TcpServer.hpp"
#include "UdpServer.hpp"
#include "Timer.hpp"
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
	public UdpServer<tweetadsd>
//	public TcpServer<tweetadsd>
{
public:
	void OnMessage(ChannelType& channel, IOBufferType& in)
	{
		IOBufferType out;

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
		printf("[pool:%u][%s:%d] connected.\n", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}

	void OnDisconnected(ChannelType& channel)
	{
		printf("[pool:%u][%s:%d] disconnected.\n", Pool::Instance().GetID(), inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port));
	}

	void OnTimeout()
	{
		static int i = 0;
		printf("i: %d\n", i);
		++i;
	}
};

class MyApp :
	public Application<MyApp>
{
public:
	void InitializeData()
	{
		PoolObject<Timer<void, 1> >::Instance().SetTimeout(&m_tweetadsd, 100);

		printf("initialize data ...\n");
		uint64_t i = 0;
		for(; i < 1000000; ++i)
		{
			TweetADS* pValue = g_HashTable.Hash(i, true);
			if(!pValue)
				break;

			pValue->tweetid = i;
			pValue->adnum = random() % 10;
			for(uint16_t i = 0; i < pValue->adnum; ++i)
				pValue->adbuffer[i] = random();
		}
		printf("initialize data complete(%lu).\n", i);
	}

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

		InitializeData();
		return true;
	}

	tweetadsd m_tweetadsd;
};

AppRun(MyApp);







