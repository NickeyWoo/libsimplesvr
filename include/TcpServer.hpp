/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
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
#include "Clock.hpp"

#define DEFAULT_SOCK_BACKLOG	100

template<typename ServerImplT, typename ChannelDataT = void>
class TcpServer
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

		if(-1 == listen(m_ServerInterface.m_Channel.fd, DEFAULT_SOCK_BACKLOG))
		{
			close(m_ServerInterface.m_Channel.fd);
			m_ServerInterface.m_Channel.fd = -1;
			return -1;
		}

		return 0;
	}

	void OnAcceptable(ServerInterface<ChannelDataT>* pInterface)
	{
		sockaddr_in cliAddr;
		bzero(&cliAddr, sizeof(sockaddr_in));
		socklen_t len = sizeof(sockaddr_in);

#ifdef __USE_GNU
		int clifd = accept4(pInterface->m_Channel.fd, (sockaddr*)&cliAddr, &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
		if(clifd == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
		}
#else
		int clifd = accept(pInterface->m_Channel.fd, (sockaddr*)&cliAddr, &len);
		if(clifd == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
		}

		if(SetNonblockAndCloexecFd(clifd) < 0)
		{
			close(clifd);
			throw InternalException((boost::format("[%s:%d][error] SetNonblockAndCloexecFd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
		}
#endif
		ServerInterface<ChannelDataT>* pChannelInterface = new ServerInterface<ChannelDataT>();
		pChannelInterface->m_Channel.fd = clifd;
		memcpy(&pChannelInterface->m_Channel.address, &cliAddr, sizeof(sockaddr_in));

		pChannelInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, reinterpret_cast<ServerImplT*>(this), _1);
		pChannelInterface->m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, reinterpret_cast<ServerImplT*>(this), _1);

		EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
		if(scheduler.Register(pChannelInterface, EventScheduler::PollType::POLLIN) == -1)
		{
			shutdown(pChannelInterface->m_Channel.fd, SHUT_RDWR);
			close(pChannelInterface->m_Channel.fd);
			delete pChannelInterface;
			throw InternalException((boost::format("[%s:%d][error] epoll_ctl add new sockfd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());
		}

		LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] connected process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

		this->OnConnected(pChannelInterface->m_Channel);

		LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] connected process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		char buffer[65535];
		IOBuffer in(buffer, 65535);
		pInterface->m_Channel >> in;

		if(in.GetReadSize() == 0)
		{
			LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] disconnected process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			this->OnDisconnected(pInterface->m_Channel);

			LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] disconnected process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			EventScheduler& scheduler = PoolObject<EventScheduler>::Instance();
			scheduler.UnRegister(pInterface);
			shutdown(pInterface->m_Channel.fd, SHUT_RDWR);
			close(pInterface->m_Channel.fd);
			delete pInterface;
		}
		else
		{
			LDEBUG_CLOCK_TRACE((boost::format("being tcp [%s:%d] message process.") % 
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			this->OnMessage(pInterface->m_Channel, in);

			LDEBUG_CLOCK_TRACE((boost::format("end tcp [%s:%d] message process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) % 
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
		}
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
	}

	// udp server interface
	TcpServer()
	{
#ifdef __USE_GNU
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return;
#else
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_STREAM, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return;

		if(SetNonblockAndCloexecFd(m_ServerInterface.m_Channel.fd) < 0)
		{
			close(m_ServerInterface.m_Channel.fd);
			m_ServerInterface.m_Channel.fd = -1;
			return;
		}
#endif

		int reuse = 1;
		if(-1 == setsockopt(m_ServerInterface.m_Channel.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
		{
			close(m_ServerInterface.m_Channel.fd);
			m_ServerInterface.m_Channel.fd = -1;
			return;
		}

		m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnAcceptable, reinterpret_cast<ServerImplT*>(this), _1);
	}

	virtual ~TcpServer()
	{
	}

	virtual void OnConnected(ChannelType& channel)
	{
	}

	virtual void OnDisconnected(ChannelType& channel)
	{
	}

	virtual void OnMessage(ChannelType& channel, IOBuffer& in)
	{
	}

	ServerInterface<ChannelDataT> m_ServerInterface;
};

#endif // define __TCPSERVER_HPP__
