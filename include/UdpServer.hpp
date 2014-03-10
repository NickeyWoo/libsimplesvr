/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __UDPSERVER_HPP__
#define __UDPSERVER_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"
#include "Server.hpp"
#include "EventScheduler.hpp"

#define UDPSERVER_BUFFER_SIZE	65535

template<typename ServerImplT, typename ChannelDataT = void>
class UdpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;

	static int Listen(ServerImplT& svr, EventScheduler* pScheduler, sockaddr_in& addr)
	{
		svr.m_OnMessageCallback = boost::bind(&ServerImplT::OnMessage, &svr, _1, _2);

		ServerInterface<ChannelDataT>* pInterface = &svr.m_ServerInterface;
#ifdef VERSION_OLD
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_DGRAM, 0);
#else
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
#endif
		if(pInterface->m_Channel.fd == -1)
			return -1;

		if(-1 == bind(pInterface->m_Channel.fd, (sockaddr*)&addr, sizeof(sockaddr_in)))
		{
			close(pInterface->m_Channel.fd);
			return -1;
		}

		pInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, &svr, _1);
		pInterface->m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, &svr, _1);

		pScheduler->Register(pInterface, EventScheduler::PollType::POLLIN);
		return 0;
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		socklen_t len = sizeof(sockaddr_in);

		char buffer[UDPSERVER_BUFFER_SIZE];
		ssize_t recvCount = recvfrom(pInterface->m_Channel.fd, buffer, UDPSERVER_BUFFER_SIZE, 0, (sockaddr*)&pInterface->m_Channel.address, &len);
		if(recvCount == -1)
			return;

		IOBuffer io(buffer, recvCount);
		m_OnMessageCallback(pInterface->m_Channel, io);
	}

	boost::function<void(ChannelType&, IOBuffer&)> m_OnMessageCallback;
	ServerInterface<ChannelDataT> m_ServerInterface;
};


#endif // define __UDPSERVER_HPP__
