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

#include "utility.hpp"
#include "keyutility.hpp"
#include "storage.hpp"
#include "hashtable.hpp"

#include "EventScheduler.hpp"
#include "Channel.hpp"
#include "IOBuffer.hpp"
#include "TcpServer.hpp"
#include "UdpServer.hpp"
#include "Configure.hpp"
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

		channel << out;
	}

};

class MyApp :
	public Application<MyApp>
{
public:
	void InitializeData()
	{
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
		if(!Register(m_tweetadsd, "tweetadsd_interface"))
		{
			printf("error: register server fail.\n");
			return false;
		}
	
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







