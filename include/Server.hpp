/*++
 *
 * Simple Server Library
 * Author: NickWu
 * Date: 2014-03-01
 *
--*/
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <string>
#include <exception>
#include "Channel.hpp"
#include "Log.hpp"

class InternalException :
	public std::exception
{
public:
	InternalException(const char* errorMessage);
	virtual ~InternalException() throw();
	virtual const char* what() const throw();

private:
	std::string m_ErrorMessage;
};


template<typename ChannelDataT>
class ServerInterface
{
public:
	inline void OnWriteable()
	{
		m_WriteableCallback(this);
	}

	inline void OnReadable()
	{
		m_ReadableCallback(this);
	}

	inline void OnError()
	{
		m_ErrorCallback(this);
	}

	boost::function<void(ServerInterface<ChannelDataT>*)> m_ReadableCallback;
	boost::function<void(ServerInterface<ChannelDataT>*)> m_WriteableCallback;
	boost::function<void(ServerInterface<ChannelDataT>*)> m_ErrorCallback;

	Channel<ChannelDataT> m_Channel;
};

int SetNonblockAndCloexecFd(int fd);

#endif // define __SERVER_HPP__
