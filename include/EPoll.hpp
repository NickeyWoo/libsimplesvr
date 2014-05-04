/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __EPOLL_HPP__
#define __EPOLL_HPP__

#include <sys/epoll.h>
#include <boost/noncopyable.hpp>
#include "Server.hpp"

class EPoll :
	public boost::noncopyable
{
public:
	EPoll();
	~EPoll();

	enum {
		ADD = EPOLL_CTL_ADD,
		MOD = EPOLL_CTL_MOD,
		DEL = EPOLL_CTL_DEL
	};

	enum {
		POLLIN = EPOLLIN,
		POLLOUT = EPOLLOUT,
		POLLERR = EPOLLERR,
		POLLHUP = EPOLLHUP
	};

	int CreatePoll();
	void Close();
	int EventCtl(int opeartor, uint32_t events, int fd, void* ptr);

	template<typename DataT>
	inline int WaitEvent(ServerInterface<DataT>** ppInterface, uint32_t* pEvents, int timeout)
	{
		epoll_event ev;
		bzero(&ev, sizeof(epoll_event));

		int ret = epoll_wait(m_epfd, &ev, 1, timeout);

		*ppInterface = (ServerInterface<DataT>*)ev.data.ptr;
		*pEvents = ev.events;
		return ret;
	}

private:
	int m_epfd;
};

#endif // define __EPOLL_HPP__
