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

template<typename ServerImplT, typename ChannelDataT = void>
class UdpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;

	static int Listen(ServerImplT& svr, EventScheduler* pScheduler, sockaddr_in& addr)
	{
		svr.m_OnMessageCallback = boost::bind(&ServerImplT::OnMessage, &svr, _1, _2);

		ServerInterface<ChannelDataT>* pInterface = &svr.m_ServerInterface;
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
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
		char buffer[65535];
		ssize_t recvCount = recvfrom(pInterface->m_Channel.fd, buffer, 65535, 0, NULL, NULL);
		if(recvCount == -1)
			return;

		IOBuffer io(buffer, recvCount);
		m_OnMessageCallback(pInterface->m_Channel, io);
	}

	boost::function<void(ChannelType&, IOBuffer&)> m_OnMessageCallback;
	ServerInterface<ChannelDataT> m_ServerInterface;
};


#endif // define __UDPSERVER_HPP__
