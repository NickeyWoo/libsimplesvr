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
#include "Server.hpp"

InternalException::InternalException(const char* errorMessage) :
	m_ErrorMessage(errorMessage)
{
}

InternalException::~InternalException() throw()
{
}

const char* InternalException::what() const throw()
{
	return m_ErrorMessage.c_str();
}


