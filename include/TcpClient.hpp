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

	static int Connect(ServerImplT& svr, EventScheduler* pScheduler, sockaddr_in& addr)
	{
		return 0;
	}

	void Disconnect()
	{
		shutdown(m_ServerInterface.m_Channel.fd, SHUT_RDWR);
		close(m_ServerInterface.m_Channel.fd);
	}

	boost::function<void(ChannelType&)> m_OnConnectedCallback;
	boost::function<void(ChannelType&, IOBufferType&)> m_OnMessageCallback;

	ServerInterface<ChannelDataT> m_ServerInterface;
	EventScheduler* m_pScheduler;
};

#endif // define __TCPCLIENT_HPP__
