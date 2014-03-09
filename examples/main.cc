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

class MyApp :
	public Application<MyApp>
{
public:
	bool Initialize(int argc, char* argv[])
	{
		if(!Register(m_echod, "echod_interface") ||
			!Register(m_discardd, "discardd_interface"))
			return false;
	
		// other initialize

		return true;
	}

	echod m_echod;
	discardd m_discardd;
};

AppRun(MyApp);



