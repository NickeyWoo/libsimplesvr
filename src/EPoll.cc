/*++
 *
 * Simple Server Library
 * Author: NickeyWoo
 * Date: 2014-03-19
 *
--*/
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
#include "EPoll.hpp"

EPoll::EPoll() :
	m_epfd(-1)
{
}

EPoll::~EPoll()
{
}

int EPoll::CreatePoll()
{
#ifdef __USE_GNU
	m_epfd = epoll_create1(EPOLL_CLOEXEC);
#else
	m_epfd = epoll_create(1000);
#endif
	return m_epfd;
}

void EPoll::Close()
{
	close(m_epfd);
}

int EPoll::EventCtl(int opeartor, uint32_t events, int fd, void* ptr)
{
	epoll_event ev;
	ev.events = events;
	ev.data.ptr = ptr;
	return epoll_ctl(m_epfd, opeartor, fd, &ev);
}


