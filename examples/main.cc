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
#include "IOBuffer.hpp"
#include "TcpServer.hpp"
#include "UdpServer.hpp"
#include "Configure.hpp"
#include "Application.hpp"

struct ServerStatus
{
	int status;
};

class echod :
	public TcpServer<echod, ServerStatus>
{
public:
	void OnConnected(ChannelType& channel)
	{
		channel.data.status = 0;
		
		sockaddr_in cliAddr = channel.GetPeerName();
		printf("[%s:%d status:%d] connected.\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), channel.data.status);
	}

	void OnDisconnected(ChannelType& channel)
	{
		sockaddr_in cliAddr = channel.GetPeerName();
		printf("[%s:%d status:%d] disconnected.\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), channel.data.status);

		channel.data.status = 0;
	}

	void OnMessage(ChannelType& channel, IOBuffer& buffer)
	{
		++channel.data.status;

		sockaddr_in cliAddr = channel.GetPeerName();
		printf("[%s:%d status:%d] say:\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), channel.data.status);
	}
};

class discardd :
	public UdpServer<discardd>
{
public:
	void OnMessage(ChannelType& channel, IOBuffer& buffer)
	{
		sockaddr_in cliAddr = channel.GetPeerName();
		printf("[%s:%d] say:\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
	}
};

struct Member {
	uint32_t	Uin;
	char		Name[16];
	time_t		RegisterTime;
} __attribute__((packed));

class MyApp :
	public Application<MyApp>
{
public:
	bool Initialize(int argc, char* argv[])
	{
		if(!Register(m_echod, "echod_interface") ||
			!Register(m_discardd, "discardd_interface"))
		{
			printf("error: register server fail.\n");
			return false;
		}
	
		// other initialize
		
		std::map<std::string, std::string> stStorageConfig = Configure::Get("storage");
		Seed seed(atoi(stStorageConfig["seed"].c_str()), atoi(stStorageConfig["count"].c_str()));

		FileStorage fs;
		if(FileStorage::OpenStorage(&fs, stStorageConfig["path"].c_str(), HashTable<uint32_t, Member>::GetBufferSize(seed)) < 0)
		{
			printf("error: open hashtable storage fail.\n");
			return false;
		}

		m_HashTable = HashTable<uint32_t, Member>::LoadHashTable(fs, seed);
		return true;
	}

	echod m_echod;
	discardd m_discardd;

	HashTable<uint32_t, Member> m_HashTable;
};

AppRun(MyApp);



