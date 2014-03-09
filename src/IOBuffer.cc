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

ssize_t IOBuffer::Write(const char* buffer, size_t size, size_t pos)
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

ssize_t IOBuffer::Read(char* buffer, size_t size, size_t pos)
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
	Read(&cbyte, sizeof(char));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (char cbyte)
{
	Write(&cbyte, sizeof(char));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (unsigned char& cbyte)
{
	Read((char*)&cbyte, sizeof(unsigned char));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (unsigned char cbyte)
{
	Write((char*)&cbyte, sizeof(unsigned char));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int16_t& wVal)
{
	Read((char*)&wVal, sizeof(int16_t));
	wVal = ntohs(wVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int16_t wVal)
{
	wVal = htons(wVal);
	Write((char*)&wVal, sizeof(int16_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint16_t& wVal)
{
	Read((char*)&wVal, sizeof(uint16_t));
	wVal = ntohs(wVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint16_t wVal)
{
	wVal = htons(wVal);
	Write((char*)&wVal, sizeof(uint16_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int32_t& dwVal)
{
	Read((char*)&dwVal, sizeof(int32_t));
	dwVal = ntohl(dwVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int32_t dwVal)
{
	dwVal = htonl(dwVal);
	Write((char*)&dwVal, sizeof(int32_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint32_t& dwVal)
{
	Read((char*)&dwVal, sizeof(uint32_t));
	dwVal = ntohl(dwVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint32_t dwVal)
{
	dwVal = htonl(dwVal);
	Write((char*)&dwVal, sizeof(uint32_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (int64_t& ddwVal)
{
	Read((char*)&ddwVal, sizeof(int64_t));
	ddwVal = ntohll(ddwVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (int64_t ddwVal)
{
	ddwVal = htonll(ddwVal);
	Write((char*)&ddwVal, sizeof(int64_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (uint64_t& ddwVal)
{
	Read((char*)&ddwVal, sizeof(uint64_t));
	ddwVal = ntohll(ddwVal);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (uint64_t ddwVal)
{
	ddwVal = htonll(ddwVal);
	Write((char*)&ddwVal, sizeof(uint64_t));
	return *this;
}

template<>
IOBuffer& IOBuffer::operator >> (std::string& str)
{
	uint16_t len = 0;
	Read((char*)&len, sizeof(uint16_t));
	len = ntohs(len);

	char* buffer = (char*)malloc(len);

	Read(buffer, len);
	str = std::string(buffer, len);

	free(buffer);
	return *this;
}

template<>
IOBuffer& IOBuffer::operator << (std::string str)
{
	uint16_t len = htons(str.length());
	Write((char*)&len, sizeof(uint16_t));
	Write(str.c_str(), str.length());
	return *this;
}




