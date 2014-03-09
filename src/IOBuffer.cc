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

IOBuffer::IOBuffer(char* buffer, size_t size) :
	m_Position(0),
	m_BufferSize(size),
	m_Buffer(buffer)
{
}

ssize_t IOBuffer::Write(const char* buffer, size_t size)
{
	return 0;
}

ssize_t IOBuffer::Write(const char* buffer, size_t size, size_t pos)
{
	return 0;
}

ssize_t IOBuffer::Read(char* buffer, size_t size)
{
	return 0;
}

ssize_t IOBuffer::Read(char* buffer, size_t size, size_t pos)
{
	return 0;
}


