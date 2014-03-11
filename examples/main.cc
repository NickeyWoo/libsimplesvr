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

struct Member {
	uint32_t	Uin;
	char		Name[16];
	time_t		RegisterTime;
} __attribute__((packed));

HashTable<uint32_t, Member> g_HashTable;

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
		
		printf("[%s:%d status:%d] connected.\n", inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port), channel.data.status);
	}

	void OnDisconnected(ChannelType& channel)
	{
		printf("[%s:%d status:%d] disconnected.\n", inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port), channel.data.status);

		channel.data.status = 0;
	}

	void OnMessage(ChannelType& channel, IOBuffer& buffer)
	{
		++channel.data.status;

		printf("[%s:%d status:%d] say:\n", inet_ntoa(channel.address.sin_addr), ntohs(channel.address.sin_port), channel.data.status);
	}

} g_echod;

struct GetMemberRequest {
	uint16_t wLen;
	uint32_t Uin[50];
};

class discardd :
	public UdpServer<discardd>
{
public:
	void OnMessage(ChannelType& channel, IOBuffer& in)
	{
		char buffer[65535];
		IOBuffer out(buffer, 65535);

		GetMemberRequest request;

		in >> request.wLen;
		out << request.wLen;

		uint16_t i = 0;
		for(; i < request.wLen; ++i)
		{
			in >> request.Uin[i];

			Member* pValue = g_HashTable.Hash(request.Uin[i]);
			if(!pValue)
				continue;

			out << pValue->Uin;
			out << std::string(pValue->Name);
			out << pValue->RegisterTime;
		}
		out.Write((char*)&i, sizeof(uint16_t), 0);
		
		sendto(channel.fd, buffer, 100, 0, (sockaddr*)&channel.address, sizeof(sockaddr_in));
	}

} g_discardd;

class MyApp :
	public Application<MyApp>
{
public:
	void InitializeData()
	{
		printf("initialize data ...\n");
		uint32_t i = 0;
		for(; i < 1000000; ++i)
		{
			Member* pMember = g_HashTable.Hash(i, true);
			if(!pMember)
				break;

			pMember->Uin = i;
			strcpy(pMember->Name, (boost::format("%08u") % i).str().c_str());
			pMember->RegisterTime = time(NULL);
		}
		printf("initialize data complete(%u).\n", i);
	}

	bool Initialize(int argc, char* argv[])
	{
		if(!Register(g_echod, "echod_interface") ||
			!Register(g_discardd, "discardd_interface"))
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
						HashTable<uint32_t, Member>::GetBufferSize(seed)) < 0)
		{
			printf("error: open hashtable storage fail.\n");
			return false;
		}

		g_HashTable = HashTable<uint32_t, Member>::LoadHashTable(sms, seed);

		InitializeData();
		return true;
	}
};

AppRun(MyApp);







