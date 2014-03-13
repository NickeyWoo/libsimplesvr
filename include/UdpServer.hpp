/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __UDPSERVER_HPP__
#define __UDPSERVER_HPP__

#include <utility>
#include <string>
#include <exception>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "Channel.hpp"
#include "Server.hpp"
#include "IOBuffer.hpp"
#include "EventScheduler.hpp"

template<typename ServerImplT, typename ChannelDataT = void, size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
class UdpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;
	typedef IOBuffer<IOBufferSize> IOBufferType;

	static int Listen(ServerImplT& svr, EventScheduler* pScheduler, sockaddr_in& addr)
	{
		svr.m_OnMessageCallback = boost::bind(&ServerImplT::OnMessage, &svr, _1, _2);

		ServerInterface<ChannelDataT>* pInterface = &svr.m_ServerInterface;
#ifdef __USE_GNU
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
#else
		pInterface->m_Channel.fd = socket(PF_INET, SOCK_DGRAM, 0);
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

		if(pScheduler->Register(pInterface, EventScheduler::PollType::POLLIN) == -1)
		{
			close(pInterface->m_Channel.fd);
			return -1;
		}
		return 0;
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		IOBufferType in;
		pInterface->m_Channel >> in;

		LDEBUG_CLOCK_TRACE((boost::format("being udp [%s:%d] message process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

		m_OnMessageCallback(pInterface->m_Channel, in);

		LDEBUG_CLOCK_TRACE((boost::format("end udp [%s:%d] message process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
	}

	ServerInterface<ChannelDataT> m_ServerInterface;
	boost::function<void(ChannelType&, IOBufferType&)> m_OnMessageCallback;
};


#endif // define __UDPSERVER_HPP__
