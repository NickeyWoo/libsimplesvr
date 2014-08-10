/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
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
#include "Clock.hpp"

template<typename ServerImplT, typename ChannelDataT = void>
class UdpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;

	int Listen(sockaddr_in& addr)
	{
		if(m_ServerInterface.m_Channel.fd == -1)
			return -1;

		if(-1 == bind(m_ServerInterface.m_Channel.fd, (sockaddr*)&addr, sizeof(sockaddr_in)))
		{
			close(m_ServerInterface.m_Channel.fd);
			m_ServerInterface.m_Channel.fd = -1;
			return -1;
		}
		return 0;
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		char buffer[65535];
		IOBuffer in(buffer, 65535);

		pInterface->m_Channel >> in;

		LDEBUG_CLOCK_TRACE((boost::format("being udp [%s:%d] message process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

		this->OnMessage(pInterface->m_Channel, in);

		LDEBUG_CLOCK_TRACE((boost::format("end udp [%s:%d] message process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
	}

	// udp server interface
	UdpServer()
	{
#ifdef __USE_GNU
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return;
#else
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_DGRAM, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return;

		if(SetNonblockAndCloexecFd(m_ServerInterface.m_Channel.fd) < 0)
		{
			close(m_ServerInterface.m_Channel.fd);
			m_ServerInterface.m_Channel.fd = -1;
			return;
		}
#endif
		m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, reinterpret_cast<ServerImplT*>(this), _1);
		m_ServerInterface.m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, reinterpret_cast<ServerImplT*>(this), _1);
	}

	virtual ~UdpServer()
	{
	}

	virtual void OnMessage(ChannelType& channel, IOBuffer& in)
	{
	}

	inline ssize_t Send(IOBuffer& out, sockaddr_in& target)
	{
		return sendto(m_ServerInterface.m_Channel.fd, 
					out.GetWriteBuffer(), out.GetWriteSize(), 0,
					(const sockaddr*)&target, sizeof(sockaddr_in));
	}

	inline ssize_t Recv(IOBuffer& in)
	{
		m_ServerInterface->m_Channel >> in;
		return in.GetReadSize();
	}

	ServerInterface<ChannelDataT> m_ServerInterface;
};


#endif // define __UDPSERVER_HPP__
