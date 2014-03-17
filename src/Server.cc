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

int SetNonblockAndCloexecFd(int fd)
{
	int sf = fcntl(fd, F_GETFL, 0);
	if(sf == -1)
		return -1;

	if(-1 == fcntl(fd, F_SETFL, sf | O_NONBLOCK))
		return -1;

	int df = fcntl(fd, F_GETFD, 0);
	if(df == -1)
		return -1;

	if(-1 == fcntl(fd, F_SETFD, df | FD_CLOEXEC))
		return -1;

	return 0;
}

