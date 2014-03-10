#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "IOBuffer.hpp"
#include "Server.hpp"

#ifndef ntohll
	#define ntohll(val)	\
			((uint64_t)ntohl(0xFFFFFFFF&val) << 32 | ntohl((0xFFFFFFFF00000000&val) >> 32))
#endif

#ifndef htonll
	#define htonll(val)	\
			((uint64_t)htonl(0xFFFFFFFF&val) << 32 | htonl((0xFFFFFFFF00000000&val) >> 32))
#endif

IOBuffer::IOBuffer(char* buffer, size_t size) :
	m_Position(0),
	m_BufferSize(size),
	m_Buffer(buffer)
{
}

ssize_t IOBuffer::Write(const char* buffer, size_t size)
{
	if(!m_Buffer || !buffer || size == 0)
		return 0;

	if(m_Position + size > m_BufferSize)
		size = m_BufferSize - m_Position;
	
	memcpy(m_Buffer + m_Position, buffer, size);
	m_Position += size;
	return size;
}

ssize_t IOBuffer::Write(const char* buffer, size_t size, size_t pos) const
{
	if(!m_Buffer || !buffer || size == 0 || pos > m_BufferSize)
		return 0;

	if(pos + size > m_BufferSize)
		size = m_BufferSize - pos;
	
	memcpy(m_Buffer + pos, buffer, size);
	return size;
}

ssize_t IOBuffer::Read(char* buffer, size_t size)
{
	if(!m_Buffer || !buffer || size == 0)
		return 0;
	
	if(m_Position + size > m_BufferSize)
		size = m_BufferSize - m_Position;
	
	memcpy(buffer, m_Buffer + m_Position, size);
	m_Position += size;
	return size;
}

ssize_t IOBuffer::Read(char* buffer, size_t size, size_t pos) const
{
	if(!m_Buffer || !buffer || size == 0 || pos > m_BufferSize)
		return 0;

	if(pos + size > m_BufferSize)
		size = m_BufferSize - pos;
	
	memcpy(buffer, m_Buffer + pos, size);
	return size;
}

template<>
IOBuffer& IOBuffer::operator >> (char& cbyte)
{
	if(m_Position + 1 > m_BufferSize)
		return *this;

	cbyte = m_Buffer[m_Position];
	++m_Position;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (char cbyte)
{
	if(m_Position + 1 > m_BufferSize)
		return *this;

	m_Buffer[m_Position] = cbyte;
	++m_Position;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (unsigned char& cbyte)
{
	if(m_Position + 1 > m_BufferSize)
		return *this;

	cbyte = (unsigned char)m_Buffer[m_Position];
	++m_Position;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (unsigned char cbyte)
{
	if(m_Position + 1 > m_BufferSize)
		return *this;

	m_Buffer[m_Position] = (char)cbyte;
	++m_Position;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int16_t& wVal)
{
	if(m_Position + 2 > m_BufferSize)
		return *this;

	wVal = *((int16_t*)(m_Buffer + m_Position));
	wVal = ntohs(wVal);
	m_Position += 2;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int16_t wVal)
{
	if(m_Position + 2 > m_BufferSize)
		return *this;

	wVal = htons(wVal);
	*((int16_t*)(m_Buffer + m_Position)) = wVal;
	m_Position += 2;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint16_t& wVal)
{
	if(m_Position + 2 > m_BufferSize)
		return *this;

	wVal = *((uint16_t*)(m_Buffer + m_Position));
	wVal = ntohs(wVal);
	m_Position += 2;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint16_t wVal)
{
	if(m_Position + 2 > m_BufferSize)
		return *this;

	wVal = htons(wVal);
	*((uint16_t*)(m_Buffer + m_Position)) = wVal;
	m_Position += 2;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int32_t& dwVal)
{
	if(m_Position + 4 > m_BufferSize)
		return *this;

	dwVal = *((int32_t*)(m_Buffer + m_Position));
	dwVal = ntohl(dwVal);
	m_Position += 4;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int32_t dwVal)
{
	if(m_Position + 4 > m_BufferSize)
		return *this;

	dwVal = htonl(dwVal);
	*((int32_t*)(m_Buffer + m_Position)) = dwVal;
	m_Position += 4;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint32_t& dwVal)
{
	if(m_Position + 4 > m_BufferSize)
		return *this;

	dwVal = *((uint32_t*)(m_Buffer + m_Position));
	dwVal = ntohl(dwVal);
	m_Position += 4;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint32_t dwVal)
{
	if(m_Position + 4 > m_BufferSize)
		return *this;

	dwVal = htonl(dwVal);
	*((uint32_t*)(m_Buffer + m_Position)) = dwVal;
	m_Position += 4;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int64_t& ddwVal)
{
	if(m_Position + 8 > m_BufferSize)
		return *this;

	ddwVal = *((int64_t*)(m_Buffer + m_Position));
	ddwVal = ntohll(ddwVal);
	m_Position += 8;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int64_t ddwVal)
{
	if(m_Position + 8 > m_BufferSize)
		return *this;

	ddwVal = htonll(ddwVal);
	*((int64_t*)(m_Buffer + m_Position)) = ddwVal;
	m_Position += 8;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint64_t& ddwVal)
{
	if(m_Position + 8 > m_BufferSize)
		return *this;

	ddwVal = *((uint64_t*)(m_Buffer + m_Position));
	ddwVal = ntohll(ddwVal);
	m_Position += 8;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint64_t ddwVal)
{
	if(m_Position + 8 > m_BufferSize)
		return *this;

	ddwVal = htonll(ddwVal);
	*((uint64_t*)(m_Buffer + m_Position)) = ddwVal;
	m_Position += 8;
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (std::string& str)
{
	if(m_Position + 2 > m_BufferSize)
		return *this;

	uint16_t wlen = *((uint16_t*)(m_Buffer + m_Position));
	wlen = ntohs(wlen);

	if(m_Position + 2 + wlen > m_BufferSize)
		return *this;

	m_Position += 2;

	char* buffer = (char*)malloc(wlen);
	memcpy(buffer, (m_Buffer + m_Position), wlen);
	m_Position += wlen;

	str = std::string(buffer, wlen);
	free(buffer);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (std::string str)
{
	if(m_Position + 2 + str.length() > m_BufferSize)
		return *this;

	uint16_t wlen = htons(str.length());
	*((uint16_t*)(m_Buffer + m_Position)) = wlen;
	m_Position += 2;

	memcpy((m_Buffer + m_Position), str.c_str(), str.length());
	m_Position += wlen;
	return *this;
}




