/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __TCPSERVER_HPP__
#define __TCPSERVER_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"
#include "Server.hpp"
#include "EventScheduler.hpp"

#define DEFAULT_SOCK_BACKLOG	100
#define TCPSERVER_BUFFER_SIZE	1024*1024

template<typename ServerImplT, typename ChannelDataT = void>
class TcpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;

	static int Listen(ServerImplT& svr, EventScheduler* pScheduler, sockaddr_in& addr)
	{
		svr.m_OnConnectedCallback = boost::bind(&ServerImplT::OnConnected, &svr, _1);
		svr.m_OnDisconnectedCallback = boost::bind(&ServerImplT::OnDisconnected, &svr, _1);
		svr.m_OnMessageCallback = boost::bind(&ServerImplT::OnMessage, &svr, _1, _2);

		ServerInterface<ChannelDataT>* pInterface = &svr.m_ServerInterface;
#ifdef VERSION_OLD
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_STREAM, 0);
#else
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
#endif
		if(pInterface->m_Channel.fd == -1)
			return -1;

		int reuse = 1;
		if(-1 == setsockopt(pInterface->m_Channel.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
		{
			close(pInterface->m_Channel.fd);
			return -1;
		}

		if(-1 == bind(pInterface->m_Channel.fd, (sockaddr*)&addr, sizeof(sockaddr_in)))
		{
			close(pInterface->m_Channel.fd);
			return -1;
		}

		if(-1 == listen(pInterface->m_Channel.fd, DEFAULT_SOCK_BACKLOG))
		{
			close(pInterface->m_Channel.fd);
			return -1;
		}

		pInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnAcceptable, &svr, _1);
		
		svr.m_pScheduler = pScheduler;
		svr.m_pScheduler->Register(pInterface, EventScheduler::PollType::POLLIN);
		return 0;
	}

	ssize_t Read(Channel<ChannelDataT>* pChannel, IOBuffer& io)
	{
		return recv(pChannel->fd, buffer, 65535, 0);
	}

	ssize_t Write(Channel<ChannelDataT>* pChannel, IOBuffer& io)
	{
		return send(pChannel->fd, buffer, 65535, 0);
	}

	void OnAcceptable(ServerInterface<ChannelDataT>* pInterface)
	{
		socklen_t len = sizeof(sockaddr_in);

		ServerInterface<ChannelDataT>* pChannelInterface = new ServerInterface<ChannelDataT>();
#ifdef VERSION_OLD
		pChannelInterface->m_Channel.fd = accept(pInterface->m_Channel.fd, (sockaddr*)&pChannelInterface->m_Channel.address, &len);
#else
		pChannelInterface->m_Channel.fd = accept4(pInterface->m_Channel.fd, (sockaddr*)&pChannelInterface->m_Channel.address, &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
#endif

		pChannelInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, this, _1);
		pChannelInterface->m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, this, _1);

		pChannelInterface->m_Channel.IOReader = boost::bind(&ServerImplT::Read, this, _1, _2);
		pChannelInterface->m_Channel.IOWriter = boost::bind(&ServerImplT::Write, this, _1, _2);

		m_pScheduler->Register(pChannelInterface, EventScheduler::PollType::POLLIN);

		LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] connected process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
		m_OnConnectedCallback(pChannelInterface->m_Channel);
		LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] connected process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		char buffer[TCPSERVER_BUFFER_SIZE];
		ssize_t recvCount = recv(pInterface->m_Channel.fd, buffer, TCPSERVER_BUFFER_SIZE, 0);
		if(recvCount == -1)
			return;
		else if(recvCount == 0)
		{
			LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] disconnected process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
			m_OnDisconnectedCallback(pInterface->m_Channel);
			LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] disconnected process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			m_pScheduler->UnRegister(pInterface);
			shutdown(pInterface->m_Channel.fd, SHUT_RDWR);
			close(pInterface->m_Channel.fd);
			delete pInterface;
		}
		else
		{
			IOBuffer io(buffer, recvCount);
			LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] message process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
			m_OnMessageCallback(pInterface->m_Channel, io);
			LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] message process.") % inet_ntoa(pInterface->m_Channel.address.sin_addr) % ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
		}
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
	}

	boost::function<void(ChannelType&)> m_OnConnectedCallback;
	boost::function<void(ChannelType&)> m_OnDisconnectedCallback;
	boost::function<void(ChannelType&, IOBuffer&)> m_OnMessageCallback;

	ServerInterface<ChannelDataT> m_ServerInterface;
	EventScheduler* m_pScheduler;
};

#endif // define __TCPSERVER_HPP__
