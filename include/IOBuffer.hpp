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
#include "Log.hpp"

#ifndef ntohll
	#define ntohll(val)	\
			((uint64_t)ntohl(0xFFFFFFFF&val) << 32 | ntohl((0xFFFFFFFF00000000&val) >> 32))
#endif

#ifndef htonll
	#define htonll(val)	\
			((uint64_t)htonl(0xFFFFFFFF&val) << 32 | htonl((0xFFFFFFFF00000000&val) >> 32))
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

class IOBuffer :
	public boost::noncopyable
{
public:
	IOBuffer(size_t size);
	IOBuffer(char* buffer, size_t size);
	~IOBuffer();

	ssize_t Write(const char* buffer, size_t size);
	ssize_t Write(const char* buffer, size_t size, size_t pos);
	ssize_t Read(char* buffer, size_t size);
	ssize_t Read(char* buffer, size_t size, size_t pos) const;
	void Dump(std::string& str);

	inline void Dump()
	{
		std::string strHexDump;
		Dump(strHexDump);
		printf("%s", strHexDump.c_str());
	}

	inline char* GetWriteBuffer()
	{
		return m_Buffer;
	}

	inline char* GetReadBuffer()
	{
		return &m_Buffer[m_ReadPosition];
	}

	inline size_t GetReadSize()
	{
		return m_AvailableReadSize;
	}

	inline size_t GetWriteSize()
	{
		return m_WritePosition;
	}

	inline size_t GetReadPosition()
	{
		return m_ReadPosition;
	}

	inline size_t GetWritePosition()
	{
		return m_WritePosition;
	}

	inline void ReadSeek(ssize_t offset)
	{
		m_ReadPosition += offset;
		if(m_ReadPosition > m_AvailableReadSize)
			m_ReadPosition = m_AvailableReadSize;
	}

	inline void WriteSeek(ssize_t offset)
	{
		m_WritePosition += offset;
		if(m_WritePosition > m_BufferSize)
			m_WritePosition = m_BufferSize;
	}

	bool m_NeedFree;
	char* m_Buffer;
	size_t m_BufferSize;
	size_t m_ReadPosition;
	size_t m_AvailableReadSize;
	size_t m_WritePosition;

private:
	int GetLeftAlignSize(long long int llNum);
};

IOBuffer& operator >> (IOBuffer& io, char& val);
IOBuffer& operator >> (IOBuffer& io, unsigned char& val);
IOBuffer& operator >> (IOBuffer& io, int16_t& val);
IOBuffer& operator >> (IOBuffer& io, uint16_t& val);
IOBuffer& operator >> (IOBuffer& io, int32_t& val);
IOBuffer& operator >> (IOBuffer& io, uint32_t& val);
IOBuffer& operator >> (IOBuffer& io, int64_t& val);
IOBuffer& operator >> (IOBuffer& io, uint64_t& val);
IOBuffer& operator >> (IOBuffer& io, std::string& val);

IOBuffer& operator << (IOBuffer& io, char val);
IOBuffer& operator << (IOBuffer& io, unsigned char val);
IOBuffer& operator << (IOBuffer& io, int16_t val);
IOBuffer& operator << (IOBuffer& io, uint16_t val);
IOBuffer& operator << (IOBuffer& io, int32_t val);
IOBuffer& operator << (IOBuffer& io, uint32_t val);
IOBuffer& operator << (IOBuffer& io, int64_t val);
IOBuffer& operator << (IOBuffer& io, uint64_t val);
IOBuffer& operator << (IOBuffer& io, std::string val);

template<typename ChannelDataT>
Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer& io)
{
	msghdr msg;
	bzero(&msg, sizeof(msghdr));

	msg.msg_name = &channel.address;
	msg.msg_namelen = sizeof(sockaddr_in);

	iovec iov;
	iov.iov_base = io.m_Buffer;
	iov.iov_len = io.m_BufferSize;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ssize_t recvSize = recvmsg(channel.fd, &msg, 0);
	if(recvSize == -1)
		throw InternalException((boost::format("[%s:%d][error] recvmsg fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

	io.m_AvailableReadSize = recvSize;
	io.m_ReadPosition = 0;
	return channel;
}

template<typename ChannelDataT>
inline Channel<ChannelDataT>& operator >> (Channel<ChannelDataT>& channel, IOBuffer* pIOBuffer)
{
	return channel >> *pIOBuffer;
}

template<typename ChannelDataT>
Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer& io)
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
		throw InternalException((boost::format("[%s:%d][error] sendmsg fail, %s.") % __FILE__ % __LINE__ % safe_strerror(errno)).str().c_str());

	io.m_WritePosition = 0;
	return channel;
}

template<typename ChannelDataT>
inline Channel<ChannelDataT>& operator << (Channel<ChannelDataT>& channel, IOBuffer* pIOBuffer)
{
	return channel << *pIOBuffer;
}

#endif // define __IOBUFFER_HPP__
