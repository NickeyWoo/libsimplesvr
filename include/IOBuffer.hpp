/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __IOBUFFER_HPP__
#define __IOBUFFER_HPP__

#include <boost/noncopyable.hpp>
#include <utility>
#include <vector>
#include <string>
#include <exception>
#include "Channel.hpp"

#ifndef ntohll
	#define ntohll(val)	\
			((uint64_t)ntohl(0xFFFFFFFF&val) << 32 | ntohl((0xFFFFFFFF00000000&val) >> 32))
#endif

#ifndef htonll
	#define htonll(val)	\
			((uint64_t)htonl(0xFFFFFFFF&val) << 32 | htonl((0xFFFFFFFF00000000&val) >> 32))
#endif

#ifndef IOBUFFER_DEFAULT_SIZE
	#define IOBUFFER_DEFAULT_SIZE		65535
#endif

template<size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer() :
		m_ReadPosition(0),
		m_WritePosition(0)
	{
	}

	template<typename T>
	IOBuffer& operator >> (T& val);

	template<typename ChannelDataT>
	IOBuffer& operator >> (Channel<ChannelDataT>& channel)
	{
		if(m_ReadPosition <= m_WritePosition)
			return *this;

		msghdr msg;
		bzero(&msg, sizeof(msghdr));

		msg.msg_name = &channel.address;
		msg.msg_namelen = sizeof(sockaddr_in);

		iovec iov;
		iov.iov_base = m_Buffer + m_WritePosition;
		iov.iov_len = m_ReadPosition - m_WritePosition;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		ssize_t sendSize = sendmsg(channel.fd, &msg, 0);
		if(sendSize == -1)
			return *this;

		m_WritePosition += sendSize;
		return *this;
	}

	template<typename T>
	IOBuffer& operator << (T val);

	template<typename ChannelDataT>
	IOBuffer& operator << (Channel<ChannelDataT>& channel)
	{
		if(m_ReadPosition >= IOBufferSize)
			return *this;

		msghdr msg;
		bzero(&msg, sizeof(msghdr));

		msg.msg_name = &channel.address;
		msg.msg_namelen = sizeof(sockaddr_in);

		iovec iov;
		iov.iov_base = m_Buffer + m_ReadPosition;
		iov.iov_len = IOBufferSize - m_ReadPosition;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		ssize_t recvSize = recvmsg(channel.fd, &msg, 0);
		if(recvSize == -1)
			return *this;

		m_ReadPosition += recvSize;
		return *this;
	}

protected:
	size_t m_ReadPosition;
	size_t m_WritePosition;
	char m_Buffer[IOBufferSize];
};

template<typename ChannelDataT, size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
inline Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>& io)
{
	io << channel;
	return channel;
}

template<typename ChannelDataT, size_t IOBufferSize = IOBUFFER_DEFAULT_SIZE>
inline Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>& io)
{
	io >> channel;
	return channel;
}

#endif // define __IOBUFFER_HPP__
