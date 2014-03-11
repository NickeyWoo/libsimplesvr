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
#include "Clock.hpp"
#include "IOBuffer.hpp"

template<typename ChannelDataT>
struct Channel {
	int				fd;
	sockaddr_in		address;
	ChannelDataT	data;
};
template<>
struct Channel<void> {
	int 			fd;
	sockaddr_in		address;
};

template<typename ChannelDataT>
class ServerInterface
{
public:

	inline void OnWriteable()
	{
		m_WriteableCallback(this);
	}

	inline void OnReadable()
	{
		m_ReadableCallback(this);
	}

	boost::function<void(ServerInterface<ChannelDataT>*)> m_ReadableCallback;
	boost::function<void(ServerInterface<ChannelDataT>*)> m_WriteableCallback;

	Channel<ChannelDataT> m_Channel;
};


#endif // define __SERVER_HPP__
