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

IOException::IOException(const char* errorMessage) :
	m_ErrorMessage(errorMessage)
{
}

IOException::~IOException() throw()
{
}

const char* IOException::what() const throw()
{
	return m_ErrorMessage.c_str();
}

OverflowIOException::OverflowIOException(const char* errorMessage) :
	IOException(errorMessage)
{
}

OverflowIOException::~OverflowIOException() throw()
{
}

const char* OverflowIOException::what() const throw()
{
	return this->m_ErrorMessage.c_str();
}








