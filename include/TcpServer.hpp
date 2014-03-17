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

#define DEFAULT_SOCK_BACKLOG	100

template<typename ServerImplT, typename ChannelDataT = void, size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
class TcpServer
{
public:
	typedef Channel<ChannelDataT> ChannelType;
	typedef IOBuffer<IOBufferSize> IOBufferType;

	int Listen(sockaddr_in& addr)
	{
#ifdef __USE_GNU
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return -1;
#else
		m_ServerInterface.m_Channel.fd = socket(PF_INET, SOCK_STREAM, 0);
		if(m_ServerInterface.m_Channel.fd == -1)
			return -1;

		if(SetNonblockAndCloexecFd(m_ServerInterface.m_Channel.fd) < 0)
		{
			close(m_ServerInterface.m_Channel.fd);
			return -1;
		}
#endif

		int reuse = 1;
		if(-1 == setsockopt(m_ServerInterface.m_Channel.fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)))
		{
			close(m_ServerInterface.m_Channel.fd);
			return -1;
		}

		if(-1 == bind(m_ServerInterface.m_Channel.fd, (sockaddr*)&addr, sizeof(sockaddr_in)))
		{
			close(m_ServerInterface.m_Channel.fd);
			return -1;
		}

		if(-1 == listen(m_ServerInterface.m_Channel.fd, DEFAULT_SOCK_BACKLOG))
		{
			close(m_ServerInterface.m_Channel.fd);
			return -1;
		}

		m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnAcceptable, this, _1);
		return 0;
	}

	void OnAcceptable(ServerInterface<ChannelDataT>* pInterface)
	{
		socklen_t len = sizeof(sockaddr_in);

		ServerInterface<ChannelDataT>* pChannelInterface = new ServerInterface<ChannelDataT>();
#ifdef __USE_GNU
		pChannelInterface->m_Channel.fd = accept4(pInterface->m_Channel.fd, (sockaddr*)&pChannelInterface->m_Channel.address, &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
		if(pChannelInterface->m_Channel.fd == -1)
		{
			delete pChannelInterface;
			throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());
		}
		printf("pid_t:%d, fd:%d\n", getpid(), pChannelInterface->m_Channel.fd);
#else
		pChannelInterface->m_Channel.fd = accept(pInterface->m_Channel.fd, (sockaddr*)&pChannelInterface->m_Channel.address, &len);
		if(pChannelInterface->m_Channel.fd == -1)
		{
			delete pChannelInterface;
			throw InternalException((boost::format("[%s:%d][error] accept fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());
		}

		if(SetNonblockAndCloexecFd(pChannelInterface->m_Channel.fd) < 0)
		{
			close(pChannelInterface->m_Channel.fd);
			delete pChannelInterface;
			throw InternalException((boost::format("[%s:%d][error] SetNonblockAndCloexecFd fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());
		}
#endif

		pChannelInterface->m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, this, _1);
		pChannelInterface->m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, this, _1);

		EventScheduler& scheduler = EventScheduler::Instance();
		if(scheduler.Register(pChannelInterface, EventScheduler::PollType::POLLIN) == -1)
		{
			shutdown(pChannelInterface->m_Channel.fd, SHUT_RDWR);
			close(pChannelInterface->m_Channel.fd);
			delete pChannelInterface;
			throw InternalException((boost::format("[%s:%d][error] epoll_ctl add new sockfd fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());
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
		IOBufferType in;
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

			EventScheduler& scheduler = EventScheduler::Instance();
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
	virtual void OnConnected(ChannelType& channel)
	{
	}

	virtual void OnDisconnected(ChannelType& channel)
	{
	}

	virtual void OnMessage(ChannelType& channel, IOBufferType& in)
	{
	}

	ServerInterface<ChannelDataT> m_ServerInterface;
};

#endif // define __TCPSERVER_HPP__
