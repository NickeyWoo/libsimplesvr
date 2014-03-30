/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-14
 *
--*/
#ifndef __TCPCLIENT_HPP__
#define __TCPCLIENT_HPP__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"
#include "Server.hpp"
#include "EventScheduler.hpp"

template<typename ServerImplT, typename ChannelDataT = void, size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
class TcpClient
{
public:
	typedef Channel<ChannelDataT> ChannelType;
	typedef IOBuffer<IOBufferSize> IOBufferType;

	int Connect(sockaddr_in& addr)
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
		m_ServerInterface.m_ReadableCallback = boost::bind(&ServerImplT::OnReadable, this, _1);
		m_ServerInterface.m_WriteableCallback = boost::bind(&ServerImplT::OnWriteable, this, _1);

		if(connect(m_ServerInterface.m_Channel.fd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1 && 
			errno != EINPROGRESS)
		{
			close(m_ServerInterface.m_Channel.fd);
			return -1;
		}
		return 0;
	}

	void Disconnect()
	{
		shutdown(m_ServerInterface.m_Channel.fd, SHUT_RDWR);
		close(m_ServerInterface.m_Channel.fd);
	}

	void OnWriteable(ServerInterface<ChannelDataT>* pInterface)
	{
		socklen_t len = sizeof(sockaddr_in);
		if(getpeername(pInterface->m_Channel.fd, (sockaddr*)&pInterface->m_Channel.address, &len) == -1)
			throw InternalException((boost::format("[%s:%d][error] getpeername fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

		EventScheduler& scheduler = EventScheduler::Instance();
		if(scheduler.Update(this, EventScheduler::PollType::POLLIN) == -1)
			throw InternalException((boost::format("[%s:%d][error] epoll_ctl update sockfd fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

		LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] connected process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

		this->OnConnected(pInterface->m_Channel);

		LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] connected process.") %
								inet_ntoa(pInterface->m_Channel.address.sin_addr) %
								ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
	}

	void OnReadable(ServerInterface<ChannelDataT>* pInterface)
	{
		IOBufferType in;
		pInterface->m_Channel >> in;

		if(in.GetReadSize() == 0)
		{
			LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] disconnected process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			this->OnDisconnected(pInterface->m_Channel);

			LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] disconnected process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			EventScheduler& scheduler = EventScheduler::Instance();
			scheduler.UnRegister(this);
			shutdown(pInterface->m_Channel.fd, SHUT_RDWR);
			close(pInterface->m_Channel.fd);
		}
		else
		{
			LDEBUG_CLOCK_TRACE((boost::format("being client [%s:%d] message process.") % 
									inet_ntoa(pInterface->m_Channel.address.sin_addr) %
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());

			this->OnMessage(pInterface->m_Channel, in);

			LDEBUG_CLOCK_TRACE((boost::format("end client [%s:%d] message process.") %
									inet_ntoa(pInterface->m_Channel.address.sin_addr) % 
									ntohs(pInterface->m_Channel.address.sin_port)).str().c_str());
		}
	}

	// tcp client interface
	virtual ~TcpClient()
	{
	}

	virtual void OnConnected(ChannelType& channel)
	{
	}

	virtual void OnDisconnected(ChannelType& channel)
	{
	}

	virtual void OnMessage(ChannelType& channel, IOBufferType& in)
	{
	}

	inline ssize_t Send(IOBufferType& out)
	{
		return send(m_ServerInterface.m_Channel.fd, 
					out.m_Buffer, out.GetWriteSize(), 0);
	}

	ServerInterface<ChannelDataT> m_ServerInterface;
};

#endif // define __TCPCLIENT_HPP__