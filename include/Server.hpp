/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "IOBuffer.hpp"

template<typename ChannelDataT>
struct Channel {
	int				fd;
	ChannelDataT	data;

	sockaddr_in GetPeerName()
	{
		sockaddr_in peer;
		socklen_t len = sizeof(sockaddr_in);
		getpeername(fd, (sockaddr*)&peer, &len);
		return peer;
	}
};
template<>
struct Channel<void> {
	int 			fd;

	sockaddr_in GetPeerName()
	{
		sockaddr_in peer;
		socklen_t len = sizeof(sockaddr_in);
		getpeername(fd, (sockaddr*)&peer, &len);
		return peer;
	}
};

template<typename ChannelDataT>
class ServerInterface
{
public:

	void OnWriteable()
	{
		m_WriteableCallback(this);
	}

	void OnReadable()
	{
		m_ReadableCallback(this);
	}

	boost::function<void(ServerInterface<ChannelDataT>*)> m_ReadableCallback;
	boost::function<void(ServerInterface<ChannelDataT>*)> m_WriteableCallback;

	Channel<ChannelDataT> m_Channel;
};


#endif // define __SERVER_HPP__
