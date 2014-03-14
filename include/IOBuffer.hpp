/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __IOBUFFER_HPP__
#define __IOBUFFER_HPP__

#include <utility>
#include <vector>
#include <string>
#include <exception>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>
#include "Channel.hpp"
#include "Server.hpp"

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

class IOException :
	public std::exception
{
public:
	IOException(const char* errorMessage);
	virtual ~IOException() throw();
	virtual const char* what() const throw();

protected:
	std::string m_ErrorMessage;
};

class OverflowIOException :
	public IOException
{
public:
	OverflowIOException(const char* errorMessage);
	virtual ~OverflowIOException() throw();
	virtual const char* what() const throw();
};

template<size_t IOBufferSize>
class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer() :
		m_ReadPosition(0),
		m_AvailableReadSize(0),
		m_WritePosition(0)
	{
	}

	ssize_t Write(const char* buffer, size_t size)
	{
		if(m_WritePosition + size > IOBufferSize)
			return 0;

		memcpy(m_Buffer + m_WritePosition, buffer, size);
		m_WritePosition += size;
		return size;
	}

	ssize_t Write(const char* buffer, size_t size, size_t pos)
	{
		if(pos + size > IOBufferSize)
			return 0;

		memcpy(m_Buffer + pos, buffer, size);
		return size;
	}

	ssize_t Read(char* buffer, size_t size)
	{
		if(m_ReadPosition + size > m_AvailableReadSize)
			return 0;

		memcpy(buffer, m_Buffer + m_ReadPosition, size);
		m_ReadPosition += size;
		return size;
	}

	ssize_t Read(char* buffer, size_t size, size_t pos) const
	{
		if(pos + size > m_AvailableReadSize)
			return 0;

		memcpy(buffer, m_Buffer + pos, size);
		return size;
	}

	inline size_t GetReadSize()
	{
		return m_AvailableReadSize;
	}

	inline size_t GetWriteSize()
	{
		return m_WritePosition;
	}

	size_t m_ReadPosition;
	size_t m_AvailableReadSize;
	size_t m_WritePosition;
	char m_Buffer[IOBufferSize];
};

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, char& val)
{
	if(io.m_ReadPosition + sizeof(char) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = io.m_Buffer[io.m_ReadPosition];
	++io.m_ReadPosition;
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, unsigned char& val)
{
	if(io.m_ReadPosition + sizeof(unsigned char) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = (unsigned char)io.m_Buffer[io.m_ReadPosition];
	++io.m_ReadPosition;
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, int16_t& val)
{
	if(io.m_ReadPosition + sizeof(int16_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((int16_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohs(val);
	io.m_ReadPosition += sizeof(int16_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, uint16_t& val)
{
	if(io.m_ReadPosition + sizeof(uint16_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((uint16_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohs(val);
	io.m_ReadPosition += sizeof(uint16_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, int32_t& val)
{
	if(io.m_ReadPosition + sizeof(int32_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((int32_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohl(val);
	io.m_ReadPosition += sizeof(int32_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, uint32_t& val)
{
	if(io.m_ReadPosition + sizeof(uint32_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((uint32_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohl(val);
	io.m_ReadPosition += sizeof(uint32_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, int64_t& val)
{
	if(io.m_ReadPosition + sizeof(int64_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((int64_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohll(val);
	io.m_ReadPosition += sizeof(int64_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, uint64_t& val)
{
	if(io.m_ReadPosition + sizeof(uint64_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	val = *((uint64_t*)(io.m_Buffer + io.m_ReadPosition));
	val = ntohll(val);
	io.m_ReadPosition += sizeof(uint64_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator >> (IOBuffer<IOBufferSize>& io, std::string& val)
{
	if(io.m_ReadPosition + sizeof(uint16_t) > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	uint16_t len = *((uint16_t*)(io.m_Buffer + io.m_ReadPosition));
	len = ntohs(len);

	if(io.m_ReadPosition + sizeof(uint16_t) + len > io.m_AvailableReadSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to read.") % __FILE__ % __LINE__).str().c_str());

	io.m_ReadPosition += sizeof(uint16_t);

	char* str = (char*)malloc(len);
	memcpy(str, (io.m_Buffer + io.m_ReadPosition), len);
	io.m_ReadPosition += len;

	val = std::string(str, len);
	free(str);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, char val)
{
	if(io.m_WritePosition + sizeof(char) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	io.m_Buffer[io.m_WritePosition] = val;
	++io.m_WritePosition;
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, unsigned char val)
{
	if(io.m_WritePosition + sizeof(char) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	io.m_Buffer[io.m_WritePosition] = (char)val;
	++io.m_WritePosition;
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, int16_t val)
{
	if(io.m_WritePosition + sizeof(int16_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htons(val);
	*((int16_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(int16_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, uint16_t val)
{
	if(io.m_WritePosition + sizeof(uint16_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htons(val);
	*((uint16_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(uint16_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, int32_t val)
{
	if(io.m_WritePosition + sizeof(int32_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htonl(val);
	*((int32_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(int32_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, uint32_t val)
{
	if(io.m_WritePosition + sizeof(uint32_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htonl(val);
	*((uint32_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(uint32_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, int64_t val)
{
	if(io.m_WritePosition + sizeof(int64_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htonll(val);
	*((int64_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(int64_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, uint64_t val)
{
	if(io.m_WritePosition + sizeof(uint64_t) > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	val = htonll(val);
	*((uint64_t*)(io.m_Buffer + io.m_WritePosition)) = val;
	io.m_WritePosition += sizeof(uint64_t);
	return io;
}

template<size_t IOBufferSize>
IOBuffer<IOBufferSize>& operator << (IOBuffer<IOBufferSize>& io, std::string val)
{
	if(io.m_WritePosition + sizeof(uint16_t) + val.length() > IOBufferSize)
		throw OverflowIOException((boost::format("[%s:%d][error] no space to write.") % __FILE__ % __LINE__).str().c_str());

	uint16_t len = htons(val.length());
	*((uint16_t*)(io.m_Buffer + io.m_WritePosition)) = len;
	io.m_WritePosition += sizeof(uint16_t);
	strcpy(io.m_Buffer + io.m_WritePosition, val.c_str());
	io.m_WritePosition += val.length();
	return io;
}

template<typename ChannelDataT, size_t IOBufferSize>
Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>& io)
{
	msghdr msg;
	bzero(&msg, sizeof(msghdr));

	msg.msg_name = &channel.address;
	msg.msg_namelen = sizeof(sockaddr_in);

	iovec iov;
	iov.iov_base = io.m_Buffer;
	iov.iov_len = IOBufferSize;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ssize_t recvSize = recvmsg(channel.fd, &msg, 0);
	if(recvSize == -1)
		throw InternalException((boost::format("[%s:%d][error] recvmsg fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());

	io.m_AvailableReadSize = recvSize;
	io.m_ReadPosition = 0;
	return channel;
}

template<typename ChannelDataT, size_t IOBufferSize>
inline Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>* pIOBuffer)
{
	return channel >> *pIOBuffer;
}

template<typename ChannelDataT, size_t IOBufferSize>
Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>& io)
{
	if(io.m_WritePosition == 0)
		return channel;

	msghdr msg;
	bzero(&msg, sizeof(msghdr));

	msg.msg_name = &channel.address;
	msg.msg_namelen = sizeof(sockaddr_in);

	iovec iov;
	iov.iov_base = io.m_Buffer;
	iov.iov_len = io.m_WritePosition;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ssize_t sendSize = sendmsg(channel.fd, &msg, 0);
	if(sendSize == -1)
		throw InternalException((boost::format("[%s:%d][error] sendmsg fail, %s.") % __FILE__ % __LINE__ % strerror(errno)).str().c_str());

	io.m_WritePosition = 0;
	return channel;
}

template<typename ChannelDataT, size_t IOBufferSize>
inline Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer<IOBufferSize>* pIOBuffer)
{
	return channel << *pIOBuffer;
}


#endif // define __IOBUFFER_HPP__
